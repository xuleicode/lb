#include <event.h>
#include <evhttp.h>
#include "pthread.h"
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <Winsock2.h>
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif // WIN32


#define MYHTTPD_SIGNATURE   "myhttpd v 0.0.1"
int httpserver_bindsocket(int port, int backlog);
int httpserver_start(int port, int nthreads, int backlog);
void* httpserver_Dispatch(void *arg);
void httpserver_GenericHandler(struct evhttp_request *req, void *arg);
void httpserver_ProcessRequest(struct evhttp_request *req);

int httpserver_bindsocket(int port, int backlog) {
	int r;
	int nfd;
	nfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nfd < 0) return -1;
	int one = 1;
	r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));//设置端口复用
	//evutil_make_listen_socket_reuseable(nfd);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	//addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	r = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
	if (r < 0) return -1;
	r = listen(nfd, backlog);
	if (r < 0) return -1;

	
	//设置非阻塞
	//evutil_make_socket_nonblocking(nfd);
#ifdef WIN32
	unsigned long flags = 1;
	if (ioctlsocket(nfd,FIONBIO,&flags) < 0)
		return -1;
#else
	int flags;
	if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
		|| fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
		return -1;
#endif
	return nfd;
}
int httpserver_start(int port, int nthreads, int backlog) {
  int r, i;
  int nfd = httpserver_bindsocket(port, backlog);
  if (nfd < 0) return -1;
  //pthread_t ths[10];
  pthread_t *ths = new pthread_t[nthreads];
  for (i = 0; i < nthreads; i++) {
    struct event_base *base = event_init();
    if (base == NULL) return -1;
    struct evhttp *httpd = evhttp_new(base);
    if (httpd == NULL) return -1;
    r = evhttp_accept_socket(httpd, nfd);
    if (r != 0) return -1;
    evhttp_set_gencb(httpd, httpserver_GenericHandler, NULL);
    r = pthread_create(ths+i, NULL, httpserver_Dispatch, base);
    if (r != 0) return -1;
  }
  for (i = 0; i < nthreads; i++) {
    pthread_join(*(ths+i), NULL);
  }
  delete[] ths;
}
 
void* httpserver_Dispatch(void *arg) {
  event_base_dispatch((struct event_base*)arg);
  return NULL;
}
 
void httpserver_GenericHandler(struct evhttp_request *req, void *arg) {
      httpserver_ProcessRequest(req);
}
 
void httpserver_ProcessRequest(struct evhttp_request *req) {
    struct evbuffer *buf = evbuffer_new();
    if (buf == NULL) return;
    
    //here comes the magic
	       char output[2048] = "\0";
        char tmp[1024];

        //获取客户端请求的URI(使用evhttp_request_uri或直接req->uri)
        const char *uri;
        uri = evhttp_request_uri(req);

        sprintf(tmp, "uri=%s\n", uri);
        strcat(output, tmp);

        sprintf(tmp, "uri=%s\n", req->uri);
        strcat(output, tmp);
        //decoded uri
        char *decoded_uri;
        decoded_uri = evhttp_decode_uri(uri);
        sprintf(tmp, "decoded_uri=%s\n", decoded_uri);
        strcat(output, tmp);

        //解析URI的参数(即GET方法的参数)
        struct evkeyvalq params;
        evhttp_parse_query(decoded_uri, &params);
        sprintf(tmp, "id=%s\n", evhttp_find_header(&params, "id"));
        strcat(output, tmp);
        sprintf(tmp, "infor=%s\n", evhttp_find_header(&params, "infor"));
        strcat(output, tmp);
        free(decoded_uri);

		char *post_datah = (char *) evhttp_find_header(req->input_headers,"Content-Type");


        //获取POST方法的数据
        char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);
		unsigned int nLength=       EVBUFFER_LENGTH(req->input_buffer);
		//*(post_data+req->body_size)=0;
        sprintf(tmp, "post_data=%s\n", post_data);
        strcat(output, tmp);

        /*
        具体的：可以根据GET/POST的参数执行相应操作，然后将结果输出
        ...
        */

        /* 输出到客户端 */

        //HTTP header
        evhttp_add_header(req->output_headers, "Server", MYHTTPD_SIGNATURE);
        evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
        evhttp_add_header(req->output_headers, "Connection", "close");
        //输出的内容
 //       struct evbuffer *buf;
  //      buf = evbuffer_new();
        evbuffer_add_printf(buf, "I got it!\n%s\n", output);
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
}
 
int main(void) {
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2) , &wsaData) != 0) 
	{
		return -1;
	}
#endif
	httpserver_start(8080, 10, 10240);
#ifdef WIN32
	WSACleanup();
#endif
}