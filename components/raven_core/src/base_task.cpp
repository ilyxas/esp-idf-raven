#include "raven_core/base_task.hpp"

#include <cassert>
#include <cstring>

namespace raven {

BaseTask::BaseTask(const Config& cfg)
    : cfg_(cfg), task_handle_(nullptr), queue_(nullptr)
{
}

BaseTask::~BaseTask()
{
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    if (queue_) {
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

void BaseTask::start()
{
    assert(queue_ == nullptr && "BaseTask::start() called more than once; queue already exists");

    queue_ = xQueueCreate(cfg_.queue_length, sizeof(TaskMessage));
    assert(queue_ != nullptr && "BaseTask: queue creation failed");

    BaseType_t result = xTaskCreate(
        &BaseTask::task_entry,
        cfg_.name,
        cfg_.stack_size,
        this,
        cfg_.priority,
        &task_handle_
    );
    assert(result == pdPASS && "BaseTask: task creation failed");
}

bool BaseTask::post_message(const TaskMessage& msg, TickType_t timeout)
{
    assert(queue_ != nullptr && "BaseTask::post_message() called before start()");

    TaskMessage copyMsg{};
    copyMsg.id = msg.id;
    copyMsg.kind = msg.kind;
    copyMsg.payload_size = msg.payload_size;
    copyMsg.data = nullptr;

    if (msg.payload_size > 0) {
        if (msg.data == nullptr) {
            return false;
        }

        copyMsg.data = pvPortMalloc(msg.payload_size);
        if (copyMsg.data == nullptr) {
            return false;
        }

        std::memcpy(copyMsg.data, msg.data, msg.payload_size);
    }

    const bool sent = (xQueueSend(queue_, &copyMsg, timeout) == pdTRUE);

    if (!sent && copyMsg.data != nullptr) {
        vPortFree(copyMsg.data);
        copyMsg.data = nullptr;
    }

    return sent;
}

// static
void BaseTask::task_entry(void* arg)
{
    static_cast<BaseTask*>(arg)->run();
}

void BaseTask::run()
{
    on_start();

    TaskMessage msg{};
    while (true) {
        if (xQueueReceive(queue_, &msg, portMAX_DELAY) == pdTRUE) {
            handle_message(msg);

            if (msg.data != nullptr) {
                vPortFree(msg.data);
                msg.data = nullptr;
            }
        }
    }
}

} // namespace raven
