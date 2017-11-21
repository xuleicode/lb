#include <event2/event.h>
//#include <event2/buffer.h>
//#include <event2/http.h>
#include <Winsock2.h>
//#include <stdlib.h>
#include <stdio.h>
//for http
#include <evhttp.h>



#define MYHTTPD_SIGNATURE   "myhttpd v 0.0.1"

void httpd_handler(struct evhttp_request *req, void *arg) {
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
	struct evbuffer *buf;
	buf = evbuffer_new();
	evbuffer_add_printf(buf, "I got it!\n%s\n", output);
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);

}
//void generic_handler(struct evhttp_request *req, void *arg)
//{
//    struct evbuffer *buf = evbuffer_new();
//    if(!buf)
//    {
//        puts("failed to create response buffer \n");
//        return;
//    }
//
//    evbuffer_add_printf(buf, "Server Responsed. Requested: %s\n", evhttp_request_get_uri(req));
//    evhttp_send_reply(req, HTTP_OK, "OK", buf);
//    evbuffer_free(buf);
//}
void specific_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *buf = evbuffer_new();
	if(!buf)
	{
		puts("failed to create response buffer \n");
		return;
	}

	evbuffer_add_printf(buf, "Server Responsed. Requested: %s\n", evhttp_request_get_uri(req));
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}
void zkfinger_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *buf = evbuffer_new();
	if(!buf)
	{
		puts("failed to create response buffer \n");
		return;
	}

	evbuffer_add_printf(buf, "Server Responsed. Requested: %s\n", evhttp_request_get_uri(req));
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}


int httpserver_bindsocket2(int port, int backlog) {

	int r;
	int nfd;
	nfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nfd < 0) return -1;
	int one = 1;
	r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));//设置端口复用

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
int main3(int argc, char* argv[])
{
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2) , &wsaData) != 0) 
	{
		return -1;
	}
#endif

	int r, i;
	int nfd = httpserver_bindsocket2(8080, 1024);
	if (nfd < 0) return -1;

	struct event_base *base = event_base_new();
	if (base == NULL) return -1;
	struct evhttp *httpd = evhttp_new(base);
	if (httpd == NULL) return -1;
	r = evhttp_accept_socket(httpd, nfd);
	if (r != 0) return -1;
	evhttp_set_gencb(httpd, zkfinger_handler, NULL);
	event_base_dispatch(base);
	
	evhttp_free(httpd);

	WSACleanup();
	return 0;
}

int main2(int argc, char* argv[])
{
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2) , &wsaData) != 0) 
	{
		return -1;
	}
#endif
	char *httpd_option_listen = "0.0.0.0";
	int httpd_option_port = 8080;
	int httpd_option_daemon = 0;
	int httpd_option_timeout = 120; //in second
	short          http_port = 8080;
	char          *http_addr = "0.0.0.0";

	struct event_base * base = event_base_new();

	struct evhttp * http_server = evhttp_new(base);
	if(!http_server)
	{
		return -1;
	}
	int ret = evhttp_bind_socket(http_server,http_addr,http_port);
	if(ret!=0)
	{
		return -1;
	}
	evhttp_set_gencb(http_server, zkfinger_handler, NULL);

	printf("http server start OK! \n");

	event_base_dispatch(base);

	evhttp_free(http_server);

	WSACleanup();
	return 0;
}
int main1(int argc, char* argv[])
{
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2) , &wsaData) != 0) 
	{
		return -1;
	}
#endif

	char *httpd_option_listen = "0.0.0.0";
	int httpd_option_port = 8080;
	int httpd_option_daemon = 0;
	int httpd_option_timeout = 120; //in seconds


	//使用libevent创建HTTP Server 
	//初始化event API
	event_init();
	//创建一个http server
	struct evhttp *httpd;
	httpd = evhttp_start(httpd_option_listen, httpd_option_port);
	evhttp_set_timeout(httpd, httpd_option_timeout);
	evhttp_set_allowed_methods( httpd , EVHTTP_REQ_GET);
	//指定generic callback
	evhttp_set_gencb(httpd, httpd_handler, NULL);
	//也可以为特定的URI指定callback
	evhttp_set_cb(httpd, "/", specific_handler, NULL);
	//也可以为特定的URI指定callback 
	evhttp_set_cb(httpd, "/zkfinger", zkfinger_handler, NULL);

	//循环处理events
	event_dispatch();

	evhttp_free(httpd);

	WSACleanup();
	return 0;
}