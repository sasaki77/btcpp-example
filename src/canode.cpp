#include "canode.h"
#include <db_access.h>
#include <stdexcept>

namespace mybt
{

    std::once_flag CaGetAction::s_ctx_once_;
    std::atomic<bool> CaGetAction::s_ctx_ready_{false};

    std::once_flag CaPutAction::s_ctx_once_;
    std::atomic<bool> CaPutAction::s_ctx_ready_{false};

    void CaGetAction::initContext()
    {
        int st = ca_context_create(ca_enable_preemptive_callback);
        s_ctx_ready_.store(st == ECA_NORMAL);
    }

    void CaGetAction::ensureContext()
    {
        std::call_once(s_ctx_once_, initContext);
        if (!s_ctx_ready_.load())
        {
            throw std::runtime_error("CA context init failed");
        }
    }

    void CaGetAction::ensureChannel(const std::string &pv)
    {
        if (chid_)
            return;
        int st = ca_create_channel(pv.c_str(), /*conn cb*/ nullptr,
                                   /*usr*/ nullptr, /*prio*/ CA_PRIORITY_DEFAULT, &chid_);
        if (st != ECA_NORMAL)
            throw std::runtime_error("ca_create_channel failed");
        st = ca_pend_io(1.0);
        if (st != ECA_NORMAL)
            throw std::runtime_error("pend_io connect timeout");
    }

    BT::NodeStatus CaGetAction::onStart()
    {
        std::cout << "caget: start" << std::endl;
        ensureContext();

        std::string pv, req_type_str;
        int count = 1, timeout_ms = 1000;
        getInput("pv_name", pv);
        getInput("req_type", req_type_str);
        getInput("count", count);
        getInput("timeout_ms", timeout_ms);

        ensureChannel(pv);

        chtype type = DBR_DOUBLE;
        if (req_type_str == "DBR_STRING")
            type = DBR_STRING;

        cancelled_.store(false);
        done_.store(false);
        ok_.store(false);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            result_ = PVdata{};
        }

        int st = ca_array_get_callback(type, static_cast<unsigned long>(count),
                                       chid_, &CaGetAction::CAGetCallback, this);
        if (st != ECA_NORMAL)
        {
            return BT::NodeStatus::FAILURE;
        }
        ca_flush_io();

        deadline_ = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus CaGetAction::onRunning()
    {
        if (done_.load())
        {
            if (ok_.load())
            {
                setOutput("result", result_);
                return BT::NodeStatus::SUCCESS;
            }
            else
            {
                return BT::NodeStatus::FAILURE;
            }
        }
        if (std::chrono::steady_clock::now() > deadline_)
        {
            return BT::NodeStatus::FAILURE;
        }
        return BT::NodeStatus::RUNNING;
    }

    void CaGetAction::onHalted()
    {
        cancelled_.store(true);
    }

    void CaGetAction::CAGetCallback(struct event_handler_args args)
    {
        auto *self = static_cast<CaGetAction *>(args.usr);
        if (!self || self->cancelled_.load())
        {
            return;
        }
        PVdata r;
        r.status = args.status;
        if (args.status == ECA_NORMAL)
        {
            if (args.type == DBR_STRING)
            {
                auto p = static_cast<const dbr_string_t *>(args.dbr);
                r.value = std::string(p[0]);
                self->ok_.store(true);
            }
            else if (args.type == DBR_DOUBLE)
            {
                auto p = static_cast<const dbr_double_t *>(args.dbr);
                r.value = p[0];
                self->ok_.store(true);
            }
            else
            {
                self->ok_.store(false);
            }
        }
        else
        {
            self->ok_.store(false);
        }
        {
            std::lock_guard<std::mutex> lk(self->mtx_);
            self->result_ = std::move(r);
        }
        self->done_.store(true);
    }

    BT::NodeStatus ShowCAResult::tick()
    {
        auto res = BT::TreeNode::getInput<PVdata>("result");

        if (!res)
        {
            throw BT::RuntimeError("missing required input [result]: ",
                                   res.error());
        }

        PVdata result = res.value();
        std::cout << "Status: " << result.status << std::endl;
        std::cout << "Status: " << PVtoString(result.value) << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    void CaPutAction::initContext()
    {
        int st = ca_context_create(ca_enable_preemptive_callback);
        s_ctx_ready_.store(st == ECA_NORMAL);
    }

    void CaPutAction::ensureContext()
    {
        std::call_once(s_ctx_once_, initContext);
        if (!s_ctx_ready_.load())
        {
            throw std::runtime_error("CA context init failed");
        }
    }

    void CaPutAction::ensureChannel(const std::string &pv)
    {
        if (chid_)
            return;
        int st = ca_create_channel(pv.c_str(), /*conn cb*/ nullptr,
                                   /*usr*/ nullptr, /*prio*/ CA_PRIORITY_DEFAULT, &chid_);
        if (st != ECA_NORMAL)
        {
            throw std::runtime_error(std::string("ca_create_channel failed: ") + ca_message(st));
        }
        // 初回のみ接続待ち（簡易実装）
        st = ca_pend_io(1.0);
        if (st != ECA_NORMAL)
        {
            throw std::runtime_error(std::string("pend_io connect timeout: ") + ca_message(st));
        }
    }

    BT::NodeStatus CaPutAction::onStart()
    {
        ensureContext();

        // 入力取得
        std::string pv;
        StrDbl val;
        int timeout_ms = 1000;
        if (!getInput("pv_name", pv))
        {
            throw BT::RuntimeError("Input [pv_name] is required");
        }
        if (!getInput("value", val))
        {
            throw BT::RuntimeError("Input [value] is required");
        }
        getInput("timeout_ms", timeout_ms);

        ensureChannel(pv);

        // 状態リセット
        cancelled_.store(false);
        done_.store(false);
        ok_.store(false);
        last_status_ = ECA_NORMAL;

        // 値の型に応じて put（完了コールバック付き）
        int st = ECA_NORMAL;
        if (std::holds_alternative<std::string>(val))
        {
            const std::string &s = std::get<std::string>(val);
            // dbr_string_t は 40 文字固定。超過は切り詰め
            std::memset(str_buf_, 0, sizeof(str_buf_));
            std::strncpy(str_buf_, s.c_str(), sizeof(str_buf_) - 1);

            st = ca_put_callback(DBR_STRING, chid_, &str_buf_, &CaPutAction::CAPutCallback, this);
        }
        else
        { // double
            dbl_buf_ = std::get<double>(val);
            st = ca_put_callback(DBR_DOUBLE, chid_, &dbl_buf_, &CaPutAction::CAPutCallback, this);
        }

        if (st != ECA_NORMAL)
        {
            setOutput("put_status", st);
            return BT::NodeStatus::FAILURE;
        }

        // リクエスト送出の後押し（バッファリングされるため）
        ca_flush_io(); // 重要：要求をネットに流すためのフラッシュ。[5](https://caffi.readthedocs.io/en/latest/intro.html)

        // タイムアウト設定
        deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus CaPutAction::onRunning()
    {
        if (done_.load())
        {
            setOutput("put_status", last_status_);
            return ok_.load() ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
        }
        if (std::chrono::steady_clock::now() > deadline_)
        {
            setOutput("put_status", ECA_TIMEOUT);
            return BT::NodeStatus::FAILURE;
        }
        return BT::NodeStatus::RUNNING;
    }

    void CaPutAction::onHalted()
    {
        // CA の put はキャンセル不可だが、後から来るコールバックを無視するための印
        cancelled_.store(true);
    }

    void CaPutAction::CAPutCallback(struct event_handler_args args)
    {
        auto *self = static_cast<CaPutAction *>(args.usr);
        if (!self || self->cancelled_.load())
            return;

        self->last_status_ = args.status;
        if (args.status == ECA_NORMAL)
        {
            self->ok_.store(true);
        }
        else
        {
            self->ok_.store(false);
        }
        self->done_.store(true);
    }
}
