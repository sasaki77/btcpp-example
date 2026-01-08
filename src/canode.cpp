#include "canode.h"
#include <db_access.h> // DBR_* 定数
#include <stdexcept>

namespace mybt
{

    std::once_flag CaGetAction::s_ctx_once_;
    std::atomic<bool> CaGetAction::s_ctx_ready_{false};

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
        st = ca_pend_io(1.0); // 初回のみ接続待ち（conn cb を使わない簡易案）
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

        // 要求型の解決（例: DBR_DOUBLEのみ実装）
        chtype type = DBR_DOUBLE;
        if (req_type_str == "DBR_STRING")
            type = DBR_STRING;
        // TODO: DBR_TIME_DOUBLE 等を必要に応じて

        // 非同期GETを発行
        cancelled_.store(false);
        done_.store(false);
        ok_.store(false);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            result_ = PVdata{};
        }
        // user_arg に this を渡す
        int st = ca_array_get_callback(type, static_cast<unsigned long>(count),
                                       chid_, &CaGetAction::CAGetCallback, this);
        if (st != ECA_NORMAL)
        {
            // 送出失敗（エンキュー失敗）
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
            return; // ハルト済みなら無視
        }
        PVdata r;
        r.status = args.status;
        if (args.status == ECA_NORMAL)
        {
            if (args.type == DBR_STRING)
            {
                auto p = static_cast<const dbr_string_t *>(args.dbr);
                r.str = std::string(p[0]);
                self->ok_.store(true);
            }
            else if (args.type == DBR_DOUBLE)
            {
                auto p = static_cast<const dbr_double_t *>(args.dbr);
                r.dbl.assign(p, p + args.count);
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
        std::cout << "Status: " << result.str << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
}
