// Copyright 2022 vlados2003
#include <csignal>
#include <unistd.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <nlohmann/json.hpp>
#include <iostream>
using nlohmann::json;

struct
{
  struct event_base *base;
  struct evhttp_connection *conn;
  bool isRunning;
} client;
void terminate([[maybe_unused]] int exit_code)
{
  client.isRunning = false;
}

void http_request_done(struct evhttp_request *req, void *arg)
{
  if(req->response_code != HTTP_OK)
  {
    std::cout << "response code: " << req->response_code << std::endl;
    //<< ", text: " << req->response_code_line << std::endl;
    event_base_loopbreak((struct event_base *)arg);
    return;
  }
  size_t len = evbuffer_get_length(req->input_buffer);
  char *str = static_cast<char *>(malloc(len + 1));
  evbuffer_copyout(req->input_buffer, str, len);
  str[len] = '\0';
  try {
    json response = json::parse(str);
    std::cout << "Perhaps you meant: " << std::endl;
    for (auto &elm : response["suggestions"])
      std::cout << "\t" << elm["text"].get<std::string>() << std::endl;
  } catch (json::parse_error &er) {
    std::cout << "request error!" << std::endl;
  }
  event_base_loopbreak((struct event_base *)arg);
}
void makeRequest(struct event_base *base, evhttp_connection *conn, json data)
{
  struct evhttp_request *req;
  req = evhttp_request_new(http_request_done, base);
  struct evbuffer *buffer = evhttp_request_get_output_buffer(req);
  //std::cout << "sending data: " << data.dump().c_str() << std::endl;
  evbuffer_add_printf(buffer, data.dump().c_str(), 8);
  evhttp_make_request(conn, req, EVHTTP_REQ_POST, "/v1/api/suggest");
  evhttp_connection_set_timeout(req->evcon, 600);
}
int main(){
  client.base = event_base_new();
  client.conn = evhttp_connection_base_new(client.base,
                                           NULL,"127.0.0.1",5555);
  client.isRunning = true;
  signal(SIGINT, terminate);
  while (client.isRunning)
  {
    std::string inp;
    std::cin >> inp;
    json data;
    data["input"] = inp;
    makeRequest(client.base, client.conn, data);
    event_base_dispatch(client.base);
  }
  return 0;
}