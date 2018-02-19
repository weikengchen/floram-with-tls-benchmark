#include "test_generic.h"

double lap;

void ocTestUtilTcpOrDieBuffered(ProtocolDesc* pd, bool isServer, const char* remote_host, const char* port, bool isProfiled, size_t first_party_send_buffer_size, size_t second_party_send_buffer_size) {
	const char key[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
	if(isServer) {
		int res;
		res = protocolAcceptTLS2P(pd, port, key, isProfiled, first_party_send_buffer_size);
		if(res!=0) {
			fprintf(stderr,"TLS accept failed\n");
			exit(1);
		}
	} else {
		int res;
		res = protocolConnectTLS2P(pd, remote_host, port, key, isProfiled, second_party_send_buffer_size);
		if (res!=0) {
			fprintf(stderr,"TLS connect failed\n");
			exit(1);
		}
	}
}	

int pma(void** dst, size_t alignment, size_t size) {
	return posix_memalign(dst, alignment, size);
}									

int main(int argc,char* argv[])
{
	srand(time(0));
	char* remote_host = NULL;
	char* port = NULL;
	bool is_server = true;

	size_t first_party_send_buffer_size;
	size_t second_party_send_buffer_size;

	// by default, the second party's send buffer size is 0 (this is also for the most cases).
	// by default, the first party's send buffer size is set to 1024 based on observation.
	first_party_send_buffer_size = 1024;
	second_party_send_buffer_size = 0;

	args_t args_pass;
	args_pass.argv = alloca(argc*sizeof(char*));
	args_pass.argv[0] = alloca(strlen(argv[0])+1);
	strcpy(args_pass.argv[0],argv[0]);
	args_pass.argc = 1;

	int arg;
	char optstring[256] = ":hc:p:y:z:";
	char * supplementary_options = get_supplementary_options_string();
	if (supplementary_options != NULL) {
		snprintf(optstring, sizeof optstring, ":hc:p:y:z:%s", supplementary_options);
	}
	while ((arg = getopt_long(argc, argv, optstring, get_long_options(), NULL)) != -1) {
		switch (arg) {
			case 'h':
				fprintf(stderr, TEXT_HELP_GENERAL);
				print_supplementary_help();
				return 1;
			case 'c':
				remote_host = alloca(strlen(optarg)+1);
				strcpy(remote_host,optarg);
				is_server = false;
				break;
			case 'p':
				port = alloca(strlen(optarg)+1);
				strcpy(port,optarg);
				break;
			case 'y':
				sscanf(optarg, "%zu", &first_party_send_buffer_size);
				break;
			case 'z':
				sscanf(optarg, "%zu", &second_party_send_buffer_size);
				break;
			case '?':
			case ':':
				if (arg == 'c' || arg == 'p') {
					fprintf (stderr, "Option -%c requires an argument.\n", arg);
					return 1;
				} else {
					fprintf (stderr, "Unrecognized option -%c.\n", optopt);
					return 1;
				}
				break;
			default:
				args_pass.argv[args_pass.argc] = alloca(3*sizeof(char));
				sprintf(args_pass.argv[args_pass.argc], "-%c", arg);
				args_pass.argc++;
				if (optarg) {
					args_pass.argv[args_pass.argc] = alloca(strlen(optarg)+1);
					strcpy(args_pass.argv[args_pass.argc],optarg);
					args_pass.argc++;
				}
		}
	}

	if (remote_host == NULL) {
		remote_host = (char *) default_remote_host;
	}
	if (port == NULL) {
		port = (char *) default_port;
	}

	printf("The first party's send buffer size is %zu.\n", first_party_send_buffer_size);
        printf("The second party's send buffer size is %zu.\n", second_party_send_buffer_size);

	ProtocolDesc pd;
	ocTestUtilTcpOrDieBuffered(&pd,is_server,remote_host,port,(strstr(get_test_name(), "benchmark") != NULL), first_party_send_buffer_size, second_party_send_buffer_size);
	setCurrentParty(&pd,is_server?1:2);

	lap = (double)current_timestamp()/1000000;
	fprintf(stderr,"Executing test: %s\n", get_test_name());
	fprintf(stderr,"Role: %s\n", (is_server?"1/Server":"2/Client"));
	execYaoProtocol(&pd,test_main,&args_pass);
	fprintf(stderr,"Total time: %lf s\n",(double)current_timestamp()/1000000-lap);

	cleanupProtocol(&pd);
	return 0;
}
