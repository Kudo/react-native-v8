#include "V8Inspector.h"

#include <sstream>

namespace facebook {

static const char kInspectorName[] = "React Native V8 Inspector";

namespace {
std::string ToSTLString(
    v8::Isolate *isolate,
    const v8_inspector::StringView &stringView) {
  int length = static_cast<int>(stringView.length());
  v8::MaybeLocal<v8::String> v8StringNullable =
      (stringView.is8Bit()
           ? v8::String::NewFromOneByte(
                 isolate,
                 reinterpret_cast<const uint8_t *>(stringView.characters8()),
                 v8::NewStringType::kNormal,
                 length)
           : v8::String::NewFromTwoByte(
                 isolate,
                 reinterpret_cast<const uint16_t *>(stringView.characters16()),
                 v8::NewStringType::kNormal,
                 length));
  if (v8StringNullable.IsEmpty()) {
    return std::string();
  }
  v8::Local<v8::String> v8String = v8StringNullable.ToLocalChecked();
  v8::String::Utf8Value utf8V8String(isolate, v8String);
  if (*utf8V8String)
    return std::string(*utf8V8String, utf8V8String.length());
  else
    return std::string();
}

v8_inspector::StringView ToStringView(const std::string &string) {
  return v8_inspector::StringView(
      reinterpret_cast<const uint8_t *>(string.data()), string.size());
}

} // namespace

InspectorFrontend::InspectorFrontend(
    InspectorClient *client,
    v8::Local<v8::Context> context)
    : client_(client) {
  isolate_ = context->GetIsolate();
}

void InspectorFrontend::sendResponse(
    int callId,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  client_->SendRemoteMessage(message->string());
}

void InspectorFrontend::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  client_->SendRemoteMessage(message->string());
}

class LocalConnection : public react::ILocalConnection {
 public:
  LocalConnection(std::weak_ptr<InspectorClient> weakClient)
      : weakClient_(weakClient) {}
  ~LocalConnection() override {}

  void sendMessage(std::string message) override {
    auto client = weakClient_.lock();
    if (!client) {
      return;
    }

    v8::HandleScope scopedIsolate(client->isolate_);
    v8_inspector::StringView messageView(
        reinterpret_cast<const uint8_t *>(message.data()), message.size());
    {
      // v8::SealHandleScope seal_scopedIsolate(client->isolate_);
      client->session_->dispatchProtocolMessage(messageView);
    }
  }

  void disconnect() override {
    auto client = weakClient_.lock();
    if (!client) {
      return;
    }

    client->Disconnect();
  }

 private:
  std::weak_ptr<InspectorClient> weakClient_;
};

int InspectorClient::nextContextGroupId_ = 1;

InspectorClient::InspectorClient(
    v8::Local<v8::Context> context,
    const std::string &appName,
    const std::string &deviceName) {
  isolate_ = context->GetIsolate();
  v8::HandleScope scopedIsolate(isolate_);
  channel_.reset(new InspectorFrontend(this, context));
  inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  inspectorName_ = CreateInspectorName(appName, deviceName);
  v8_inspector::StringView inspectorNameStringView =
      ToStringView(inspectorName_);
  int contextGroupId = nextContextGroupId_++;
  session_ = inspector_->connect(
      contextGroupId, channel_.get(), inspectorNameStringView);
  context_.Reset(isolate_, context);

  inspector_->contextCreated(v8_inspector::V8ContextInfo(
      context, contextGroupId, inspectorNameStringView));
}

InspectorClient::~InspectorClient() {
  v8::HandleScope scopedIsolate(isolate_);
  inspector_->contextDestroyed(context_.Get(isolate_));
  Disconnect();
}

void InspectorClient::runMessageLoopOnPause(int contextGroupId) {
  paused_ = true;
  std::unique_lock<std::mutex> lock(pauseMutex_);
  pauseWaitable_.wait(lock, [this] { return !paused_; });
}

void InspectorClient::quitMessageLoopOnPause() {
  std::lock_guard<std::mutex> lock(pauseMutex_);
  paused_ = false;
  pauseWaitable_.notify_all();
}

v8::Local<v8::Context> InspectorClient::ensureDefaultContextInGroup(
    int group_id) {
  return context_.Get(isolate_);
}

void InspectorClient::ConnectToReactFrontend() {
  std::lock_guard<std::mutex> lock(connectionMutex_);

  auto weakClient = CreateWeakPtr();
  pageId_ = react::getInspectorInstance().addPage(
      inspectorName_,
      "V8",
      [weakClient = std::move(weakClient)](
          std::unique_ptr<react::IRemoteConnection> remoteConn) {
        auto client = weakClient.lock();
        if (client) {
          client->remoteConn_ = std::move(remoteConn);
        }
        return std::make_unique<LocalConnection>(weakClient);
      });
}

void InspectorClient::Disconnect() {
  std::lock_guard<std::mutex> lock(connectionMutex_);
  if (remoteConn_) {
    remoteConn_->onDisconnect();
  }
  react::getInspectorInstance().removePage(pageId_);
}

void InspectorClient::SendRemoteMessage(
    const v8_inspector::StringView &message) {
  if (remoteConn_) {
    std::string messageString = ToSTLString(isolate_, message);
    remoteConn_->onMessage(messageString);
  }
}

std::weak_ptr<InspectorClient> InspectorClient::CreateWeakPtr() {
  return std::weak_ptr<InspectorClient>(shared_from_this());
}

// static
std::string InspectorClient::CreateInspectorName(
    const std::string &appName,
    const std::string &deviceName) {
  std::ostringstream ss;
  ss << kInspectorName << " - " << appName;
  return ss.str();
}

} // namespace facebook
