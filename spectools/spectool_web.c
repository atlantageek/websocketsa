/* Spectrum tools raw interface
 *
 * Basic dumper to dump the raw values from the USB device to stdout.  Meant
 * to be fed to another app via popen (such as the python grapher).  Caller
 * is responsible for converting the RSSI values.
 *
 * Mike Kershaw/Dragorn <dragorn@kismetwireless.net>
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Extra thanks to Ryan Woodings @ Metageek for interface documentation
 */

#include <stdio.h>
#include <usb.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <libwebsockets.h>

#include "config.h"

#include "spectool_container.h"
#include "spectool_net_client.h"

#define SNAPSHOT_LEN 60

spectool_phy *devs = NULL;
int ndev = 0;
spectool_sample_sweep *ran = NULL;

struct serveable {
	const char *urlpath;
	const char *mimetype;
};

static const struct serveable whitelist[] = {
	{ "/favicon.ico", "image/x-icon" },
	{ "/libwebsockets.org-logo.png", "image/png" },

	/* last one is the default served if no match */
	{ "/index.js", "text/html" },
	{ "/index.html", "text/html" },
	{ "/index.html", "text/html" },
};
struct per_session_data__http {
	int fd;
	
};

struct per_session_data__trace {
	int start_freq;
	int stop_freq;
	int trace[1000];
	
};


static int callback_http(struct libwebsocket_context *context,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
	char buf[2560];
	char leaf_path[1024];
	char resource_path[1024]="./webui";
	int n, m,result;
        char json[256];
	unsigned char *p;
	unsigned char *ext;
	unsigned char *mimetype;
	static unsigned char buffer[4096];
	struct stat stat_buf;
	struct per_session_data__http *pss =
	        (struct per_session_data__http *)user;
#ifdef EXTERNAL_POLL
	int fd = (int)(long)in;
#endif
	mimetype = malloc(30 * sizeof(char));
	switch (reason) {
	case LWS_CALLBACK_HTTP:

                printf("Callback for http\n");
		if (strlen((char *)in) < 2)
		{
                        printf("Getting index file\n");
			in="/index.html";
		}
		result = strncmp((char *)(in + 1), "config",6);
		if (result == 0)
		{
                        printf("Getting Json\n");
		 	sprintf(json, "{\"start\": %d, \"stop\":%d, \"num_samples\":%d, \"rssi_max\":%d}", ran->start_khz, ran->end_khz, ran->num_samples, ran->rssi_max );
			printf("%s\n",json);
			sprintf(buf, 
				"HTTP/1.0 200 OK\x0d\x0a"
                                "Server: libwebsockets\x0d\x0a"
                                "Content-Type: application/json\x0d\x0a"
                                        "Content-Length: %u\x0d\x0a\x0d\x0a%s", strlen(json),json);
			libwebsocket_write(wsi,buf, strlen(buf),LWS_WRITE_HTTP);
		}
		else
		{
                        printf("Getting other files\n");
			ext = strrchr(in, '.');
                        printf("Getting other files with ext %s\n",ext);
			if (strcmp(ext,".css") == 0) {
				strcpy(mimetype, "text/css");
			}
			else if (strcmp(ext,".js") == 0) {
				strcpy(mimetype, "application/javascript");
			}
			else {
				strcpy(mimetype, "text/html");
			}
			printf("%s %s %s %d\n", buf, ext, mimetype, strlen(buf));

			sprintf(buf, "%s%s", resource_path, (char *)in);
			if (libwebsockets_serve_http_file(context, wsi, buf, mimetype,NULL))
				return -1; /* through completion or error, close the socket */
		}

		/*
		 * notice that the sending of the file completes asynchronously,
		 * we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when
		 * it's done
		 */

		break;

	case LWS_CALLBACK_HTTP_FILE_COMPLETION:
//		lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen\n");
		/* kill the connection after we sent one file */
		return -1;


bail:
		close(pss->fd);
		return -1;

	/*
	 * callback for confirming to continue with client IP appear in
	 * protocol 0 callback since no websocket protocol has been agreed
	 * yet.  You can just ignore this if you won't filter on client IP
	 * since the default uhandled callback return is 0 meaning let the
	 * connection continue.
	 */

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
#if 0
		libwebsockets_get_peer_addresses(context, wsi, (int)(long)in, client_name,
		                                 sizeof(client_name), client_ip, sizeof(client_ip));

		fprintf(stderr, "Received network connect from %s (%s)\n",
		        client_name, client_ip);
#endif
		/* if we returned non-zero from here, we kill the connection */
		break;

#ifdef EXTERNAL_POLL
	/*
	 * callbacks for managing the external poll() array appear in
	 * protocol 0 callback
	 */

	case LWS_CALLBACK_ADD_POLL_FD:

		if (count_pollfds >= max_poll_elements) {
			lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
			return 1;
		}

		fd_lookup[fd] = count_pollfds;
		pollfds[count_pollfds].fd = fd;
		pollfds[count_pollfds].events = (int)(long)len;
		pollfds[count_pollfds++].revents = 0;
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		if (!--count_pollfds)
			break;
		m = fd_lookup[fd];
		/* have the last guy take up the vacant slot */
		pollfds[m] = pollfds[count_pollfds];
		fd_lookup[pollfds[count_pollfds].fd] = m;
		break;

	case LWS_CALLBACK_SET_MODE_POLL_FD:
		pollfds[fd_lookup[fd]].events |= (int)(long)len;
		break;

	case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
		pollfds[fd_lookup[fd]].events &= ~(int)(long)len;
		break;
#endif

	default:
		break;
	}

	return 0;
}
callback_trace(struct libwebsocket_context *context,
                        struct libwebsocket *wsi,
                        enum libwebsocket_callback_reasons reason,
                                               void *user, void *in, size_t len)
{
        struct per_session_data__protocol *pss = (struct per_session_data__protocol *)user;
	char json_data[100000];
        //ran->sample_data
        printf("Callback Trace\n");
	switch (reason) {
       		case LWS_CALLBACK_ESTABLISHED:
			lwsl_info("callback_dumb_increment: "
				 "LWS_CALLBACK_ESTABLISHED\n");
			printf("Callback Established\n");
			libwebsocket_callback_on_writable(context, wsi);
			break;
        	case LWS_CALLBACK_SERVER_WRITEABLE:
			curr_trace(0,NULL,json_data + LWS_SEND_BUFFER_PRE_PADDING );
			printf("Callback Writable:%d %s|\n",strlen(&json_data[LWS_SEND_BUFFER_PRE_PADDING]),&json_data[LWS_SEND_BUFFER_PRE_PADDING]);
			libwebsocket_write(wsi, (char*)&json_data[LWS_SEND_BUFFER_PRE_PADDING], strlen(&json_data[LWS_SEND_BUFFER_PRE_PADDING]) , LWS_WRITE_TEXT);
			libwebsocket_callback_on_writable(context, wsi);
			break;
        	case LWS_CALLBACK_RECEIVE:
			printf("Callback Receive\n");
			break;
        	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
			printf("Callback filter.\n");
			break;
        	default:
			printf("Callback default.\n");
			break;
	}
	return 0;


}
/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",            /* name */
		callback_http,          /* callback */
		sizeof (struct per_session_data__http), /* per_session_data_size */
		0,                      /* max frame size / rx buffer */
	},
        {
                "trace-protocol",
                callback_trace,
                sizeof(struct per_session_data__trace),
                10,
        },
	{ NULL, NULL, 0, 0 } /* terminator */
};

struct per_session_data__protocol {
        int number;
};




void sighandle(int sig) {
	int x;

	printf("Dying %d from signal %d\n", getpid(), sig);

	exit(1);
}

void Usage(void) {
	printf("spectool_raw [ options ]\n"
	       " -n / --net  tcp://host:port  Connect to network server instead of\n"
	       " -b / --broadcast             Listen for (and connect to) broadcast servers\n"
	       " -l / --list				  List devices and ranges only\n"
	       " -r / --range [device:]range  Configure a device for a specific range\n"
	       "                              local USB devices\n");
	return;
}

typedef struct
{
	int max_trace[1000];
	int min_trace[1000];
	int total_trace[1000];
	int trace_count;
} DatalogSnapshot;


void store_snapshot(time_t snapshot_start, DatalogSnapshot *snapshot) {


};
void reset_snapshot(DatalogSnapshot *snapshot) {
	printf("Reseting snapshot after %d count\n", snapshot->trace_count);
	snapshot->trace_count = 0;
}

void poll_webserver() {
}

void curr_trace(int index, int val, char *result)
{
  static int trace[1000];
  static int max_index=0;
  int i=0;
  char inner_json[20];
  inner_json[0]='\0';

  if (result == NULL)
  {
    /*So he must be setting a value  so lets return a json string*/
    trace[index]=val;
    if (index > max_index)
    {
      max_index=index;
    }
  }
  else
  {
    sprintf(result, "{\"trace\": [");
    for (i=0; i<max_index;i++)
    {
      sprintf(inner_json,"%d",trace[i]);
      if (i > 0) { strcat(result,",");}
      strcat(result,inner_json);
    }
    strcat(result,"]}");
    /*printf(" %d %d %s|\n",max_index,strlen(result), result);*/
  }
}

int main(int argc, char *argv[]) {
	/*Configure web interface */
	struct libwebsocket_context *lws_context;
	int opts;
	int service_count;
	char interface_name[128] = "";
	const char *iface = NULL;

	struct lws_context_creation_info lws_info;




	/* Spectools */

	spectool_device_list list;
	int x = 0, r = 0;
	spectool_sample_sweep *sb;
	spectool_server sr;
	char errstr[SPECTOOL_ERROR_MAX];
	int ret;
	spectool_phy *pi;
	time_t snapshot_start;

	/* snapshot */
	/*DatalogSnapshot snapshot;*/

	/* Init webserver data */
	memset(&lws_info, 0, sizeof lws_info);
	lws_info.port = 7681;
	lws_info.iface = iface;
	lws_info.protocols = protocols;
	lws_info.ssl_cert_filepath = NULL;
	lws_info.ssl_private_key_filepath = NULL;
	lws_info.gid = -1;
	lws_info.uid = -1;

	lws_context = libwebsocket_create_context(&lws_info);
	if (lws_context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}


	lws_set_log_level(7, lwsl_emit_syslog);
	lwsl_notice("libwebsockets test server - "
	            "(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> - "
	            "licensed under LGPL2.1\n");




	snapshot_start = time(NULL);
	static struct option long_options[] = {
		{ "net", required_argument, 0, 'n' },
		{ "broadcast", no_argument, 0, 'b' },
		{ "list", no_argument, 0, 'l' },
		{ "range", required_argument, 0, 'r' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};
	int option_index;

	char *neturl = NULL;

	char bcasturl[SPECTOOL_NETCLI_URL_MAX];
	int bcastlisten = 0;
	int bcastsock;
	char *captured_trace = (char *) malloc(sizeof(char) * 512);
	char *strnbr=malloc(6);


	int list_only = 0;
	int *rangeset = NULL;

	ndev = spectool_device_scan(&list);
	*captured_trace='\0';

	if (ndev > 0) {
		rangeset = (int *) malloc(sizeof(int) * ndev);
		memset(rangeset, 0, sizeof(int) * ndev);
	}


	//reset_snapshot(&snapshot);
	while (1) {
		int o = getopt_long(argc, argv, "n:bhr:l",
		                    long_options, &option_index);

		if (o < 0)
			break;

		if (o == 'h') {
			Usage();
			return;
		} else if (o == 'b') {
			bcastlisten = 1;
		} else if (o == 'n') {
			neturl = strdup(optarg);
			printf("debug - spectool_raw neturl %s\n", neturl);
			continue;
		} else if (o == 'l') {
			list_only = 1;
		} else if (o == 'r' && ndev > 0) {
			if (sscanf(optarg, "%d:%d", &x, &r) != 2) {
				if (sscanf(optarg, "%d", &r) != 1) {
					fprintf(stderr, "Invalid range, expected device#:range# "
					        "or range#\n");
					exit(-1);
				} else {
					rangeset[0] = r;
				}
			} else {
				if (x < 0 || x >= ndev) {
					fprintf(stderr, "Invalid range, no device %d\n", x);
					exit(-1);
				} else {
					rangeset[x] = r;
				}
			}
		}
	}

	signal(SIGINT, sighandle);

	if (list_only) {
		if (ndev <= 0) {
			printf("No spectool devices found, bailing\n");
			exit(1);
		}

		printf("Found %d devices...\n", ndev);

		for (x = 0; x < ndev; x++) {
			printf("Device %d: %s id %u\n",
			       x, list.list[x].name, list.list[x].device_id);

			for (r = 0; r < list.list[x].num_sweep_ranges; r++) {
				ran = &(list.list[x].supported_ranges[r]);

				printf("  Range %d: \"%s\" %d%s-%d%s @ %0.2f%s, %d samples\n", r,
				       ran->name,
				       ran->start_khz > 1000 ?
				       ran->start_khz / 1000 : ran->start_khz,
				       ran->start_khz > 1000 ? "MHz" : "KHz",
				       ran->end_khz > 1000 ? ran->end_khz / 1000 : ran->end_khz,
				       ran->end_khz > 1000 ? "MHz" : "KHz",
				       (ran->res_hz / 1000) > 1000 ?
				       ((float) ran->res_hz / 1000) / 1000 : ran->res_hz / 1000,
				       (ran->res_hz / 1000) > 1000 ? "MHz" : "KHz",
				       ran->num_samples);
			}

		}

		exit(0);
	}

	if (bcastlisten) {
		printf("Initializing broadcast listen...\n");

		if ((bcastsock = spectool_netcli_initbroadcast(SPECTOOL_NET_DEFAULT_PORT,
		                                               errstr)) < 0) {
			printf("Error initializing bcast socket: %s\n", errstr);
			exit(1);
		}

		printf("Waiting for a broadcast server ID...\n");
	} else if (neturl != NULL) {
		printf("Initializing network connection...\n");

		if (spectool_netcli_init(&sr, neturl, errstr) < 0) {
			printf("Error initializing network connection: %s\n", errstr);
			exit(1);
		}

		if (spectool_netcli_connect(&sr, errstr) < 0) {
			printf("Error opening network connection: %s\n", errstr);
			exit(1);
		}

		printf("Connected to server, waiting for device list...\n");
	} else if (neturl == NULL) {
		if (ndev <= 0) {
			printf("No spectool devices found, bailing\n");
			exit(1);
		}

		printf("Found %d spectool devices...\n", ndev);

		for (x = 0; x < ndev; x++) {
			printf("Initializing WiSPY device %s id %u\n",
			       list.list[x].name, list.list[x].device_id);

			pi = (spectool_phy *) malloc(SPECTOOL_PHY_SIZE);
			pi->next = devs;
			devs = pi;

			if (spectool_device_init(pi, &(list.list[x])) < 0) {
				printf("Error initializing WiSPY device %s id %u\n",
				       list.list[x].name, list.list[x].device_id);
				printf("%s\n", spectool_get_error(pi));
				exit(1);
			}

			if (spectool_phy_open(pi) < 0) {
				printf("Error opening WiSPY device %s id %u\n",
				       list.list[x].name, list.list[x].device_id);
				printf("%s\n", spectool_get_error(pi));
				exit(1);
			}

			spectool_phy_setcalibration(pi, 1);

			/* configure the default sweep block */
			spectool_phy_setposition(pi, rangeset[x], 0, 0);
		}

		spectool_device_scan_free(&list);
	}

	//redis_c = redisConnect("127.0.0.1", 6379);
	/* Naive poll that doesn't use select() to find pending data */
	while (1) {
		fd_set rfds;
		fd_set wfds;
		int maxfd = 0;
		struct timeval tm;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);


		pi = devs;
		while (pi != NULL) {
			if (spectool_phy_getpollfd(pi) >= 0) {
				FD_SET(spectool_phy_getpollfd(pi), &rfds);

				if (spectool_phy_getpollfd(pi) > maxfd)
					maxfd = spectool_phy_getpollfd(pi);
			}

			pi = pi->next;
		}

		if (neturl != NULL) {
			if (spectool_netcli_getpollfd(&sr) >= 0) {
				FD_SET(spectool_netcli_getpollfd(&sr), &rfds);

				if (spectool_netcli_getpollfd(&sr) > maxfd)
					maxfd = spectool_netcli_getpollfd(&sr);
			}
			if (spectool_netcli_getwritepend(&sr) > 0) {
				FD_SET(spectool_netcli_getwritefd(&sr), &wfds);

				if (spectool_netcli_getwritefd(&sr) > maxfd)
					maxfd = spectool_netcli_getwritefd(&sr);
			}
		}

		if (bcastlisten) {
			FD_SET(bcastsock, &rfds);
			if (bcastsock > maxfd)
				maxfd = bcastsock;
		}

		tm.tv_sec = 0;
		tm.tv_usec = 10000;

		if (select(maxfd + 1, &rfds, &wfds, NULL, &tm) < 0) {
			printf("spectool_raw select() error: %s\n", strerror(errno));
			exit(1);
		}

		if (bcastlisten && FD_ISSET(bcastsock, &rfds)) {
			if (spectool_netcli_pollbroadcast(bcastsock, bcasturl, errstr) == 1) {
				printf("Saw broadcast for server %s\n", bcasturl);

				if (neturl == NULL) {
					neturl = strdup(bcasturl);

					if (spectool_netcli_init(&sr, neturl, errstr) < 0) {
						printf("Error initializing network connection: %s\n", errstr);
						exit(1);
					}

					if (spectool_netcli_connect(&sr, errstr) < 0) {
						printf("Error opening network connection: %s\n", errstr);
						exit(1);
					}
				}
			}
		}

		if (neturl != NULL && spectool_netcli_getwritefd(&sr) >= 0 &&
		    FD_ISSET(spectool_netcli_getwritefd(&sr), &wfds)) {
			if (spectool_netcli_writepoll(&sr, errstr) < 0) {
				printf("Error write-polling network server %s\n", errstr);
				exit(1);
			}
		}

		ret = SPECTOOL_NETCLI_POLL_ADDITIONAL;
		while (neturl != NULL && spectool_netcli_getpollfd(&sr) >= 0 &&
		       FD_ISSET(spectool_netcli_getpollfd(&sr), &rfds) &&
		       (ret & SPECTOOL_NETCLI_POLL_ADDITIONAL)) {

			if ((ret = spectool_netcli_poll(&sr, errstr)) < 0) {
				printf("Error polling network server %s\n", errstr);
				exit(1);
			}

			if ((ret & SPECTOOL_NETCLI_POLL_NEWDEVS)) {
				spectool_net_dev *ndi = sr.devlist;
				while (ndi != NULL) {
					printf("Enabling network device: %s (%u)\n", ndi->device_name,
					       ndi->device_id);
					pi = spectool_netcli_enabledev(&sr, ndi->device_id, errstr);

					pi->next = devs;
					devs = pi;

					ndi = ndi->next;
				}

			}
		}

		pi = devs;
		while (pi != NULL) {
			spectool_phy *di = pi;
			pi = pi->next;

			if (spectool_phy_getpollfd(di) < 0) {
				if (spectool_get_state(di) == SPECTOOL_STATE_ERROR) {
					printf("Error polling spectool device %s\n",
					       spectool_phy_getname(di));
					printf("%s\n", spectool_get_error(di));
					exit(1);
				}

				continue;
			}

			if (FD_ISSET(spectool_phy_getpollfd(di), &rfds) == 0) {
				continue;
			}



			do {
				r = spectool_phy_poll(di);

				if ((r & SPECTOOL_POLL_CONFIGURED)) {
					printf("Configured device %u (%s)\n",
					       spectool_phy_getdevid(di),
					       spectool_phy_getname(di),
					       di->device_spec->num_sweep_ranges);

					ran = spectool_phy_getcurprofile(di);

					if (ran == NULL) {
						printf("Error - no current profile?\n");
						continue;
					}

					/*printf("    %d%s-%d%s @ %0.2f%s, %d samples\n",
					           ran->start_khz > 1000 ?
					           ran->start_khz / 1000 : ran->start_khz,
					           ran->start_khz > 1000 ? "MHz" : "KHz",
					           ran->end_khz > 1000 ? ran->end_khz / 1000 : ran->end_khz,
					           ran->end_khz > 1000 ? "MHz" : "KHz",
					           (ran->res_hz / 1000) > 1000 ?
					                ((float) ran->res_hz / 1000) / 1000 : ran->res_hz / 1000,
					           (ran->res_hz / 1000) > 1000 ? "MHz" : "KHz",
					           ran->num_samples);*/
					//redisCommand(redis_c, "SET wispy:config:start_freq %d", ran->start_khz);
					//redisCommand(redis_c, "SET wispy:config:stop_freq %d", ran->end_khz);

					continue;
				} else if ((r & SPECTOOL_POLL_ERROR)) {
					printf("Error polling spectool device %s\n",
					       spectool_phy_getname(di));
					printf("%s\n", spectool_get_error(di));
					exit(1);
				} else if ((r & SPECTOOL_POLL_SWEEPCOMPLETE)) {
					sb = spectool_phy_getsweep(di);
					if (sb == NULL)
						continue;
					/*printf("%s: ", spectool_phy_getname(di));*/

					//if ((time(NULL) - snapshot_start) > SNAPSHOT_LEN)
					//{
						//store_snapshot(snapshot_start, &snapshot);
						//reset_snapshot(&snapshot);
						//snapshot_start = time(NULL);
						//snapshot.trace_count = 0;
					//}

					for (r = 0; r < sb->num_samples; r++) {
						sprintf(strnbr, ",%d ",
						        SPECTOOL_RSSI_CONVERT(sb->amp_offset_mdbm, sb->amp_res_mdbm, sb->sample_data[r]));
						strcat(captured_trace,strnbr);
						curr_trace(r,SPECTOOL_RSSI_CONVERT(sb->amp_offset_mdbm, sb->amp_res_mdbm, sb->sample_data[r]),(char *)NULL);

						//if ( snapshot.trace_count == 0 )
						//{
							//snapshot.max_trace[r] = sb->sample_data[r];
							//snapshot.min_trace[r] = sb->sample_data[r];
							//snapshot.total_trace[r] = sb->sample_data[r];
						//}
						//else
						//{
							//if ( snapshot.max_trace[r] < sb->sample_data[r] )
								//snapshot.max_trace[r] = sb->sample_data[r];
							//if ( snapshot.min_trace[r] > sb->sample_data[r] )
								//snapshot.min_trace[r] = sb->sample_data[r];
							//snapshot.total_trace[r] = snapshot.total_trace[r] + sb->sample_data[r];
						//}
						//snapshot.trace_count = snapshot.trace_count + 1;

					}

					/*printf("%d %s\n",time(NULL), captured_trace);*/
					//redisCommand(redis_c, "LPUSH wispy %s",captured_trace);
					captured_trace[0] = '\0';
					//redis_reply = redisCommand(redis_c, "LLEN wispy ");
					//if (redis_reply->integer > 501)
					//{
					//  redisCommand(redis_c, "RPOP wispy");
					//  redisCommand(redis_c, "RPOP wispy");
					//}

					poll_webserver();
				}
			} while ((r & SPECTOOL_POLL_ADDITIONAL));

		}
		service_count = libwebsocket_service(lws_context, 50);
	}

	return 0;
}

