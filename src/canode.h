#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cadef.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>

namespace mybt
{
    struct PVdata
    {
        int status = ECA_NORMAL;
        std::vector<double> dbl; // 例: DOUBLE配列
        std::string str;         // 例: STRING
                                 // TODO: DBR_TIME_* なら timestamp / alarm を追加
    };

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
}
