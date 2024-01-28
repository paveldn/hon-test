#include "esphome.h"
#include <queue>
// HaierProtocol
#include <protocol/haier_protocol.h>

class HaierTestDevice : public Component, 
                        public UARTDevice,
                        public haier_protocol::ProtocolStream {
 public:
  HaierTestDevice(UARTComponent *parent) 
      : UARTDevice(parent),
        haier_protocol_(*this) {
  }

  void setup() override {
    this->last_message_timestamp_ = std::chrono::steady_clock::now();
    haier_protocol::set_log_handler(
      std::bind(
        &HaierTestDevice::protocol_logger_, 
        this, 
        std::placeholders::_1, 
        std::placeholders::_2,
        std::placeholders::_3
      )
    );
    this->haier_protocol_.set_default_message_handler(
      std::bind(
        &HaierTestDevice::message_handler_, 
        this, 
        std::placeholders::_1, 
        std::placeholders::_2,
        std::placeholders::_3
      )
    );
    this->haier_protocol_.set_default_answer_handler(
      std::bind(
        &HaierTestDevice::answer_handler, 
        this, 
        std::placeholders::_1, 
        std::placeholders::_2,
        std::placeholders::_3, 
        std::placeholders::_4
      )
    );
    this->haier_protocol_.set_default_timeout_handler(
      std::bind(
        &HaierTestDevice::timeout_handler_, 
        this, 
        std::placeholders::_1
      )
    );
  }

  void loop() override {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (this->can_send_message_() && !this->outgoing_queue_.empty() && this->is_message_interval_exceeded_(now)) {
      auto message = std::move(this->outgoing_queue_.front());
      this->outgoing_queue_.pop();
      this->haier_protocol_.send_message(message.first, message.second);
    }
    this->haier_protocol_.loop();
  }

  size_t available() noexcept override { 
    return esphome::uart::UARTDevice::available(); 
  };

  size_t read_array(uint8_t *data, size_t len) noexcept override {
    return esphome::uart::UARTDevice::read_array(data, len) ? len : 0;
  };

  void write_array(const uint8_t *data, size_t len) noexcept override {
    esphome::uart::UARTDevice::write_array(data, len);
  };

  void send_message(haier_protocol::HaierMessage message, bool use_crc = true) {
    this->outgoing_queue_.push(QueueItem(message, use_crc) );
  }

  void set_answer_timeout(uint32_t timeout) {
    this->haier_protocol_.set_answer_timeout(timeout);
  }

 protected:
 
  bool can_send_message_() const {
    return !this->haier_protocol_.is_waiting_for_answer() && (this->haier_protocol_.get_outgoing_queue_size() == 0);
  };

  bool is_message_interval_exceeded_(std::chrono::steady_clock::time_point now) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - last_message_timestamp_).count() > 1000;
  }


  haier_protocol::HandlerError message_handler_(haier_protocol::FrameType message_type, const uint8_t* data, size_t size) {
    switch (message_type) {
      case haier_protocol::FrameType::ALARM_STATUS:
      case haier_protocol::FrameType::REPORT:
        // Accepting these frames
        this->haier_protocol_.send_answer(haier_protocol::HaierMessage(haier_protocol::FrameType::CONFIRM));
        return haier_protocol::HandlerError::HANDLER_OK;
      default:
        // Rejecting everything else
        constexpr uint8_t invalid_command_code[] = { 0x00, 0x00 };
        this->haier_protocol_.send_answer(haier_protocol::HaierMessage(haier_protocol::FrameType::INVALID, invalid_command_code, sizeof(invalid_command_code)));
        return haier_protocol::HandlerError::UNSUPPORTED_MESSAGE;
    }
  }
 
  haier_protocol::HandlerError answer_handler(haier_protocol::FrameType request_type, haier_protocol::FrameType answer_type, const uint8_t* data, size_t size) {
    return haier_protocol::HandlerError::HANDLER_OK;
  };
  
  haier_protocol::HandlerError timeout_handler_(haier_protocol::FrameType type) {
    return haier_protocol::HandlerError::HANDLER_OK;
  };
  
  void protocol_logger_(haier_protocol::HaierLogLevel level, const char *tag, const char *message) {
    switch (level) {
      case haier_protocol::HaierLogLevel::LEVEL_ERROR:
        esp_log_printf_(ESPHOME_LOG_LEVEL_ERROR, tag, __LINE__, "%s", message);
        break;
      case haier_protocol::HaierLogLevel::LEVEL_WARNING:
        esp_log_printf_(ESPHOME_LOG_LEVEL_WARN, tag, __LINE__, "%s", message);
        break;
      case haier_protocol::HaierLogLevel::LEVEL_INFO:
        esp_log_printf_(ESPHOME_LOG_LEVEL_INFO, tag, __LINE__, "%s", message);
        break;
      case haier_protocol::HaierLogLevel::LEVEL_DEBUG:
        esp_log_printf_(ESPHOME_LOG_LEVEL_DEBUG, tag, __LINE__, "%s", message);
        break;
      case haier_protocol::HaierLogLevel::LEVEL_VERBOSE:
        esp_log_printf_(ESPHOME_LOG_LEVEL_VERBOSE, tag, __LINE__, "%s", message);
        break;
      default:
        // Just ignore everything else
        break;
    }
  }
  
  using QueueItem = std::pair<haier_protocol::HaierMessage, bool>;
  std::queue<QueueItem> outgoing_queue_;
  std::chrono::steady_clock::time_point last_message_timestamp_;
  haier_protocol::ProtocolHandler haier_protocol_;
};