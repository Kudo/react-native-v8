#pragma once

#include <condition_variable>

#include "jsinspector/InspectorInterfaces.h"
#include "v8-inspector.h"

namespace facebook {

class InspectorClient;
class ClientConnectionWrapper;

class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
 public:
  explicit InspectorFrontend(
      InspectorClient *client,
      v8::Local<v8::Context> context);
  ~InspectorFrontend() override = default;

 private:
  void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void flushProtocolNotifications() override {}

 private:
  InspectorClient *client_;
  v8::Isolate *isolate_;
  v8::Global<v8::Context> context_;
};

class InspectorClient final
    : public v8_inspector::V8InspectorClient,
      public std::enable_shared_from_this<InspectorClient> {
 public:
  InspectorClient(
      v8::Local<v8::Context> context,
      const std::string &appName,
      const std::string &deviceName);
  ~InspectorClient() override;

  void runMessageLoopOnPause(int contextGroupId) override;
  void quitMessageLoopOnPause() override;
  v8::Local<v8::Context> ensureDefaultContextInGroup(int group_id) override;

  void ConnectToReactFrontend();
  void Disconnect();
  void SendRemoteMessage(const v8_inspector::StringView &message);

  v8::Isolate *GetIsolate();
  v8_inspector::V8InspectorSession *GetInspectorSession();

 private:
  static std::string CreateInspectorName(
      const std::string &appName,
      const std::string &deviceName);

  std::weak_ptr<InspectorClient> CreateWeakPtr();

 private:
  static int nextContextGroupId_;

  std::unique_ptr<v8_inspector::V8Inspector> inspector_;
  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
  std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
  v8::Isolate *isolate_;
  v8::Global<v8::Context> context_;

  std::unique_ptr<react::IRemoteConnection> remoteConn_;
  std::string inspectorName_;

  std::mutex connectionMutex_;
  std::mutex pauseMutex_;
  std::condition_variable pauseWaitable_;
  bool paused_ = false;

  int pageId_;
  std::shared_ptr<ClientConnectionWrapper> clientConnectionWrapper_;
};

} // namespace facebook
