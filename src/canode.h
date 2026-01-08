#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>

#include <behaviortree_cpp/bt_factory.h>
#include <cadef.h>
#include <db_access.h>

using StrDbl = std::variant<std::string, double>;

namespace BT
{

    template <>
    inline StrDbl convertFromString(StringView sv)
    {
        // BT::StringView → std::string
        const std::string s = toStr(sv);

        // 数値パース（Cロケール）
        char *endp = nullptr;
        const double v = std::strtod(s.c_str(), &endp);

        // 完全一致なら double
        if (endp && *endp == '\0')
        {
            return StrDbl{v};
        }
        // それ以外は string
        return StrDbl{s};
    }
}

namespace mybt
{
    struct PVdata
    {
        int status = ECA_NORMAL;
        std::variant<
            double,
            std::string>
            value;
    };

    inline std::string PVtoString(const std::variant<double, std::string> v)
    {
        return std::visit([](const auto &value) -> std::string
                          {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else { // double
            std::ostringstream oss;
            oss << std::setprecision(8) << value;
            return oss.str();
        } }, v);
    }

    class CaGetAction : public BT::StatefulActionNode
    {
    public:
        CaGetAction(const std::string &name, const BT::NodeConfig &cfg)
            : BT::StatefulActionNode(name, cfg) {}

        static BT::PortsList providedPorts()
        {
            return {
                BT::InputPort<std::string>("pv_name"),
                BT::InputPort<std::string>("req_type", "DBR_DOUBLE"),
                BT::InputPort<int>("count", 1, "default value is 1"),
                BT::InputPort<int>("timeout_ms", 1000, "default value is 1000"),
                BT::OutputPort<PVdata>("result")};
        }

        BT::NodeStatus onStart() override;
        BT::NodeStatus onRunning() override;
        void onHalted() override;

    private:
        static void CAGetCallback(struct event_handler_args args);

        void ensureContext();
        void ensureChannel(const std::string &pv);
        static void initContext();

        // CA objects
        static std::once_flag s_ctx_once_;
        static std::atomic<bool> s_ctx_ready_;
        chid chid_ = nullptr;

        // Request state
        std::atomic<bool> done_{false};
        std::atomic<bool> ok_{false};
        std::atomic<bool> cancelled_{false};
        PVdata result_;
        std::mutex mtx_;

        // timeout
        std::chrono::steady_clock::time_point deadline_;
    };

    class ShowCAResult : public BT::SyncActionNode
    {
    public:
        ShowCAResult(const std::string &name, const BT::NodeConfig &config)
            : BT::SyncActionNode(name, config)
        {
        }

        static BT::PortsList providedPorts()
        {
            return {BT::InputPort<PVdata>("result")};
        }

        BT::NodeStatus tick() override;
    };

    class CaPutAction : public BT::StatefulActionNode
    {
    public:
        CaPutAction(const std::string &name, const BT::NodeConfig &cfg)
            : BT::StatefulActionNode(name, cfg) {}

        static BT::PortsList providedPorts()
        {
            return {
                BT::InputPort<std::string>("pv_name", "書き込み対象の PV 名"),
                BT::InputPort<StrDbl>("value", "書き込み値（string または double）"),
                BT::InputPort<int>("timeout_ms", 1000, "完了待ちタイムアウト(ms)"),
                BT::OutputPort<int>("put_status", "CA のステータス（ECA_*）")};
        }

        // StatefulActionNode のライフサイクル
        BT::NodeStatus onStart() override;
        BT::NodeStatus onRunning() override;
        void onHalted() override;

    private:
        // --- CA コンテキスト初期化（プロセス全体で一度） ---
        static void initContext();
        static void ensureContext();

        static std::once_flag s_ctx_once_;
        static std::atomic<bool> s_ctx_ready_;

        // --- チャネル確立 ---
        void ensureChannel(const std::string &pv);

        // --- put 完了コールバック ---
        static void CAPutCallback(struct event_handler_args args);

    private:
        // CHID
        chid chid_ = nullptr;

        // 要求状態
        std::atomic<bool> done_{false};
        std::atomic<bool> ok_{false};
        std::atomic<bool> cancelled_{false};
        int last_status_ = ECA_NORMAL;

        // バッファ（put が完了するまで生存させる）
        dbr_string_t str_buf_{}; // DBR_STRING 用（40文字制限）
        double dbl_buf_ = 0.0;   // DBR_DOUBLE 用

        // タイムアウト
        std::chrono::steady_clock::time_point deadline_;
    };
}
