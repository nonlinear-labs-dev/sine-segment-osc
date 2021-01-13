#include <external/WebSocketServer.h>

namespace External
{

  WebSocketServer::WebSocketServer()
  {
  }

  void WebSocketServer::rpc(const std::string &name, WebSocketServer::RPC rpc)
  {
    m_rpc[name] = rpc;
  }

  int WebSocketServer::run(int argc, char **argv)
  {
    uint port = 49999;
    auto server = soup_server_new(nullptr, nullptr);

    auto callback = (SoupServerWebsocketCallback) &WebSocketServer::webSocket;
    soup_server_add_websocket_handler(server, nullptr, nullptr, nullptr, callback, this, nullptr);
    soup_server_listen_all(server, port, SOUP_SERVER_LISTEN_IPV4_ONLY, nullptr);

    m_loop = Glib::MainLoop::create();
    m_loop->run();
    g_object_unref(server);
    return 0;
  }

  void WebSocketServer::quit()
  {
    m_loop->quit();
  }

  void WebSocketServer::webSocket(SoupServer *, SoupWebsocketConnection *c, const char *, SoupClientContext *,
                                  WebSocketServer *pThis)
  {
    g_signal_connect(c, "message", G_CALLBACK(&WebSocketServer::receiveMessage), pThis);
    g_signal_connect(c, "closed", G_CALLBACK(&WebSocketServer::onConnectionClosed), pThis);
    g_object_ref(c);
    pThis->m_connections.push_back(c);
  }

  void WebSocketServer::receiveMessage(SoupWebsocketConnection *c, gint, GBytes *message, WebSocketServer *pThis)
  {
    try
    {
      gsize len = 0;
      auto raw = static_cast<const char *>(g_bytes_get_data(message, &len));
      auto rawJson = std::string(raw, raw + len);
      nlohmann::json j = nlohmann::json::parse(rawJson);
      std::string rpcName = j.at("rpc");
      auto &rpc = pThis->m_rpc.at(rpcName);
      auto arg = j.at("arg");
      nlohmann::json res;
      res["result"] = rpc(arg);
      soup_websocket_connection_send_text(c, res.dump().c_str());
    }
    catch(...)
    {
      fprintf(stderr, "Exception when receiving websocket message");
    }
  }

  void WebSocketServer::onConnectionClosed(SoupWebsocketConnection *c, WebSocketServer *pThis)
  {
    pThis->m_connections.remove(c);
    g_object_unref(c);
  }
}