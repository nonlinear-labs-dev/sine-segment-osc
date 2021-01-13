#pragma once

#include <libsoup/soup-server.h>
#include <functional>
#include <string>
#include <map>
#include <list>
#include <gtkmm.h>
#include <nlohmann/json.hpp>

namespace External
{
  class WebSocketServer
  {
   public:
    WebSocketServer();

    // argument: { rpc: "name-of-the function", arg: { opaque json }}
    // result: { result: { opaque json }}
    using RPC = std::function<nlohmann::json(nlohmann::json)>;

    void rpc(const std::string &name, RPC rpc);
    int run(int argc, char **argv);
    void quit();

   private:
    static void webSocket(SoupServer *, SoupWebsocketConnection *c, const char *, SoupClientContext *,
                          WebSocketServer *pThis);
    static void receiveMessage(SoupWebsocketConnection *c, gint, GBytes *message, WebSocketServer *pThis);
    static void onConnectionClosed(SoupWebsocketConnection *c, WebSocketServer *pThis);

    std::map<std::string, RPC> m_rpc;
    std::list<SoupWebsocketConnection *> m_connections;
    Glib::RefPtr<Glib::MainLoop> m_loop;
  };

}
