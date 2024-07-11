#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cuda.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern char _binary_debayer_fatbin_start[];

char *custrerror(CUresult n) {
	const char *sp;
	if (cuGetErrorString(n, &sp) != CUDA_SUCCESS) { 
		return "unknown";
	} else {
		return (char *)sp;
	}
}

int main(int argc, char **argv) {
	int i, j, n, m, k = 256, b = 8192, w = 4096, h = 2160;
	char *buf;
	CUresult e;
	CUdevice dev;
	CUcontext ctx;
	CUmodule module;
	CUfunction debayer;
	CUdeviceptr cubuf;

	for (i = 1; i < argc; i++) {
		switch (argv[i][0]) {
		case '-':
			switch (argv[i][1]) {
			case 'w':
				if ((w = atoi(argv[++i])) <= 0) {
					fprintf(stderr, "failed to parse width from '%s'\n", argv[i]);
					return 1;
				}
				break;
			case 'h':
				if ((h = atoi(argv[++i])) <= 0) {
					fprintf(stderr, "failed to parse height from '%s'\n", argv[i]);
					return 1;
				}
				break;
			case 'b':
				if ((b = atoi(argv[++i])) <= 0) {
					fprintf(stderr, "failed to parse buffer size from '%s'\n", argv[i]);
					return 1;
				}
				break;
			case 'k':
				if ((k = atoi(argv[++i])) <= 0) {
					fprintf(stderr, "failed to parse kernel count from '%s'\n", argv[i]);
					return 1;
				}
				break;
			default:
				fprintf(stderr, "usage: bayer2rgb [-w width] [-h height] [-b buffer-size] [-k kernel count]");
				return 1;
			}
			
		}
	}


	if ((buf = malloc(b)) == NULL) {
		fprintf(stderr, "malloc %s\n", strerror(errno));
		return 1;
	}

	if ((e = cuInit(0)) != CUDA_SUCCESS) {
		fprintf(stderr, "loading module: %s\n", custrerror(e));
		return 1;
	}

	if ((e = cuDeviceGetCount(&n)) != CUDA_SUCCESS) {
		fprintf(stderr, "getting device count: %s\n", custrerror(e));
		return 1;
	} else if (n == 0) {
		fprintf(stderr, "no devices detected\n");
		return 1;
	}

	if ((e = cuDeviceGet(&dev, 0)) != CUDA_SUCCESS) {
		fprintf(stderr, "loading module: %s\n", custrerror(e));
		return 1;
	}

	if ((e = cuCtxCreate(&ctx, 0, dev)) != CUDA_SUCCESS) {
		fprintf(stderr, "loading module: %s\n", custrerror(e));
		return 1;
	}

	if ((e = cuModuleLoadFatBinary(&module, _binary_debayer_fatbin_start)) != CUDA_SUCCESS) {
		fprintf(stderr, "loading module: %s\n", custrerror(e));
		return 1;
	}

	if ((e = cuModuleGetFunction(&debayer, module, "debayer")) != CUDA_SUCCESS) {
		fprintf(stderr, "loading debayer function: %s\n", custrerror(e));
		return 1;
	}

	if ((e = cuMemAlloc(&cubuf, w*h*4)) != CUDA_SUCCESS) {
		fprintf(stderr, "allocating: %s\n", custrerror(e));
		return 1;
	}

	for (;;) {
		for (i = 0; i < w*h; i += n) {
			if ((n = read(0, &buf[0], MIN((w*h)-i, b))) == -1) {
				fprintf(stderr, "read error: %s\n", strerror(errno));
				return 1;
			} else if (n == 0 && i > 0) {
				fprintf(stderr, "premature EOF\n");
				return 1;
			} else if (n == 0 && i > 0) {
				return 0;
			}

			if ((e = cuMemcpyHtoD(cubuf, &buf[0], n)) != CUDA_SUCCESS) {
				fprintf(stderr, "copying HtoD: %s\n", custrerror(e));
				return 1;
			}

		}

		if ((e = cuLaunchKernel(debayer, MIN(((w*h)+k)/k,1), 1, 1, k, 1, 1, 0, 0, ((void *[]){ (void*)&cubuf, }), 0)) != CUDA_SUCCESS) {
			fprintf(stderr, "launching kernel: %s\n", custrerror(e));
			return 1;
		}

		for (i = 0, m = MIN((w*h*4)-i, b); i < w*h*4; i += m) {
			if ((e = cuMemcpyDtoH(&buf[0], (CUdeviceptr)&((char*)cubuf)[(w*h)+i], m)) != CUDA_SUCCESS) {
				fprintf(stderr, "copying DtoH: %s %d\n", custrerror(e), i);
				return 1;
			}
			for (j = 0; j < m; j += n) {
				if ((n = write(1, &buf[j], m-j)) <= 0) {
					fprintf(stderr, "write: %s\n", strerror(errno));
					return 1;
				}
			}
		}
	}
}
