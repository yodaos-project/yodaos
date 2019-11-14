
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>

#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>

#include <hardware/mic_array.h>

static inline int mic_array_device_open (const hw_module_t *module, struct mic_array_device_t **device) {
	return module->methods->open (module, MIC_ARRAY_HARDWARE_MODULE_ID, (struct hw_device_t **) device);
}

struct mic_array_module_t* module = NULL;
struct mic_array_device_t* mic_array_device = NULL;
typedef struct alg_data_ {
        char tag[8];
        int len;
} alg_data;

typedef struct frame_header_ {
        char tag[8];
        int frame_id;
        int aec_flag;
        int len;
} frame_header;

typedef struct alg_proto_header_ { //protocol header
        char tag[8];
        int frm_num;
        int sl_num;
        int mic_num;
        int frm_size;
        int bf_num;
        int fft_size;
        int len;
} alg_proto_header;
char *Buff;
char *Ext_buff;
float **aecData;
float **bbfData;
float **slData;
FILE *fp = NULL;
FILE *fp_aec = NULL;
FILE *fp_bbf = NULL;
FILE *fp_sl = NULL;
int frame = 5535;
int AecDataBufferLen = 0;
int BbfDataBufferLen = 0;
int SlDataBufferLen = 0;

void main()
{
	int ret;
	int mic_array_frame_size;
	int read_size = 8192;//1024*8
	//int ext_size = 288;//72*4
	//int ext_size = 8192;//(4 + 288 ~~ 2k/4) + 128*4*2*4 + 8k/4
	int ext_size;

	if (hw_get_module (MIC_ARRAY_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module) == 0) {
		//open mic array
		if (0 != mic_array_device_open(&module->common, &mic_array_device)) {
			return;
		}
	}

	fp = fopen("/data/mic_array_demo.pcm", "wb");
	fp_aec = fopen("/data/mic_array_aec.pcm", "wb");
	fp_bbf = fopen("/data/mic_array_bbf.pcm", "wb");
	fp_sl = fopen("/data/mic_array_sl.pcm", "wb");


	printf("File open\n");
//	mic_array_frame_size = mic_array_device->get_stream_buff_size(mic_array_device);
//	printf("mic_array_frame_size=%d\n",mic_array_frame_size);
	mic_array_device->dspMultiBf = 1;// using dsp multi bf
	mic_array_device->dspBfNum = 8;// using ctc 8-8
	mic_array_device->start_stream(mic_array_device);
	int malloc_size = mic_array_device->get_stream_buff_size(mic_array_device); 
	printf("start stream, malloc_size=%d\n", malloc_size);
	Buff = (char *)malloc(read_size);
        Ext_buff = (char *)malloc(malloc_size);

	int total = 0;
	while(frame > 0){
		#ifdef ENABLE_DSP_ALL_EXT
		ret = mic_array_device->read_stream(mic_array_device, Ext_buff, &ext_size);
                //printf("read stream, ext_size=%d\n", ext_size);
		//fwrite (Ext_buff, sizeof(char), ext_size, fp);
		parseDspData(Ext_buff, ext_size);
		#elif ENABLE_DSP_
		ret = mic_array_device->read_stream(mic_array_device, Buff, read_size, Ext_buff, ext_size);
		fwrite (Buff, sizeof(char), read_size, fp);
		#else
		ret = mic_array_device->read_stream(mic_array_device, Buff, read_size);
		fwrite (Buff, sizeof(char), read_size, fp);
		#endif
		if(ret != 0) {
			printf("read stream failed\n");
		} else {
			total += read_size;
			//printf("read stream size is=%d, total=%d\n",read_size, total);
		}
		//test
		/*for(int i = 0; i < ext_size/sizeof(float); i++)
			printf("mic_array_demo, index=%d, value=%f\n", i, ((float*)Ext_buff)[i]);*/
		frame--;
	}
	fclose(fp);
	fclose(fp_aec);
	fclose(fp_bbf);
	fclose(fp_sl);
	mic_array_device->stop_stream(mic_array_device);
}


void parseDspData(const unsigned char* data, int len) {
        const unsigned char* p = data;
        alg_proto_header * header = (alg_proto_header*) p;
	printf("parseDspData, header->tag=%s\n", header->tag);
        assert(strcmp(header->tag, "DSPDATA") == 0);

        //parse dsp data head
        const int FRM_SIZE = header->frm_size;
        const int FFT_SIZE = header->fft_size;
        const int BfNum = header->bf_num;
        int aecDataLen = header->frm_num * FRM_SIZE;
        int micNum = header->mic_num;
        checkbuffer(header->mic_num, aecDataLen, header->frm_num, header->bf_num, header->fft_size, header->sl_num);
        p += sizeof(alg_proto_header);
        const unsigned char* end = p + header->len;
        for (int i = 0; i < header->frm_num; i++) {
                frame_header* frameHeader = (frame_header*) p;
		//printf("parseDspData, frameHeader->tag=%s, frameHeader_id=%d\n", frameHeader->tag, frameHeader->frame_id);
                assert(strcmp(frameHeader->tag, "FRMDATA") == 0);
                p += sizeof(frame_header);
                const unsigned char* EndOfFrm = p + frameHeader->len;
		//printf("11111111111111111111111111111111\n");
                for (; p < EndOfFrm;) {
                        alg_data* dataHead = (alg_data*) p;
                        p += sizeof(alg_data);
			//printf("dataHead->tag=%s\n", dataHead->tag);
                        if (strcmp(dataHead->tag, "AECDATA") == 0) {
                                const int LEN = sizeof(float) * FRM_SIZE;
                                for (int j = 0; j < micNum; j++) {
					//printf("parseDspData, memcpy begin aaaaaaaaa\n");
					//nan test
					for(int t = 0; t < LEN/sizeof(float); t++) {
					    //printf("nan test aaaaaaaaaaaaaaaaaaaaaa, p[%d]=%f, isnan=%d\n", t, ((float*)p)[t], isnan(((float*)p)[t]));
					    //assert(isnan(((float*)p)[t]));
					    if(isnan(((float*)p)[t]) || isinf(((float*)p)[t])) {
						printf("warning: we met nan or inf, dataHead->tag=%s, index=%d\n", dataHead->tag, t);
					    }
					}
                                        memcpy(aecData[j] + i * FRM_SIZE, p, LEN);
					//printf("parseDspData, memcpy end aaaaaaaaa\n");
					//fwrite ((float*)(aecData[j] + i * FRM_SIZE), sizeof(float), LEN/sizeof(float), fp_aec);
					//printf("parseDspData, fwrite end aaaaaaaaaaaaaaa\n");
                                        p += LEN;
                                }
				for (int q = 0; q < FRM_SIZE; q++)
				    for (int k = 0; k < micNum; k++) {
					    fwrite ((float*)(aecData[k] + q + i * FRM_SIZE), sizeof(float), 1, fp_aec);
					    //printf("parseDspData, fwrite end aaaaaaaaaaaaaaa\n");
					}
                        } else if (strcmp(dataHead->tag, "BBFFREQ") == 0) {
                                const int LEN = sizeof(float) * FFT_SIZE;
                                for (int j = 0; j < BfNum; j++) {
					//nan test
					for(int t = 0; t < LEN/sizeof(float); t++) {
					    //printf("nan test bbbbbbbbbb, p[%d]=%f, isnan=%d\n", t, ((float*)p)[t], isnan(((float*)p)[t]));
					    //assert(isnan(((float*)p)[t]));
					    if(isnan(((float*)p)[t]) || isinf(((float*)p)[t])) {
						printf("warning: we met nan or inf, dataHead->tag=%s, index=%d\n", dataHead->tag, t);
					    }
					}
					//printf("parseDspData, memcpy begin bbbbbbbbbbb, index=%d\n", (i * BfNum + j));
                                        memcpy(bbfData[i * BfNum + j], p, LEN);
					//printf("parseDspData, memcpy end bbbbbbbbbbbb\n");
			//		fwrite((float*)(bbfData[i * BfNum + j]), sizeof(float), LEN/sizeof(float), fp_bbf);
					//printf("parseDspData, fwrite end bbbbbbbbbbbb\n");
                                        p += LEN;
                                }
                        } else if (strcmp(dataHead->tag, "SLDATA") == 0) {
                                const int LEN = sizeof(float) * header->sl_num;
				//printf("parseDspData, memcpy begin cccccccccc\n");
					//nan test
					for(int t = 0; t < LEN/sizeof(float); t++) {
					    //printf("nan test cccccccccccccc, p[%d]=%f, isnan=%d\n", t, ((float*)p)[t], isnan(((float*)p)[t]));
					    //assert(isnan(((float*)p)[t]));
					    if(isnan(((float*)p)[t]) || isinf(((float*)p)[t])) {
						printf("warning: we met nan or inf, dataHead->tag=%s, index=%d\n", dataHead->tag, t);
					    }
					}
                                memcpy(slData[i], p, LEN);
				//printf("parseDspData, memcpy end cccccccccccccc\n");
			//	fwrite((float*)slData[i], sizeof(float), LEN/sizeof(float), fp_sl);
				//printf("parseDspData, fwrite end cccccccccccc\n");
                                p += LEN;
                        } else {
                                p += dataHead->len;
                                printf("unkown tag: %s\n", p);
                        }
                }
                if (p != EndOfFrm) {
                        //printf("p>end\n");
                }
		//printf("begin assert(p == EndOfFrm)\n");
                assert(p == EndOfFrm);
        }
	//printf("begin assert(p == end)\n");
        assert(p == end);
}

void checkbuffer(int micNum, int aecDataLen, int frmNum, int bbfRow, int bbfCol, int slCol) {
	printf("checkbuffer, micNum=%d, aecDataLen=%d, frmNum=%d, bbfRow=%d, bbfCol=%d, slCol=%d\n", micNum, aecDataLen, frmNum, bbfRow, bbfCol, slCol);
        if (aecDataLen > AecDataBufferLen) {
		//printf("checkbuffer, aaaaaaaaaaaaaaaa\n");
                AecDataBufferLen = aecDataLen;
		if(aecData != NULL) {
		    free(aecData);
		}

		aecData = (float **)malloc(micNum * sizeof(float));
		for(int i=0; i<micNum; i++)
		    aecData[i] = (float*)malloc(aecDataLen * sizeof(float));
		//printf("checkbuffer, aaaaaaaaaaaaaaaa   end\n");
                //R2_DEL_AR2(aecData);
                //aecData = R2_NEW_AR2(float, micNum, aecDataLen);
        }

        if (frmNum * bbfRow > BbfDataBufferLen) {
		//printf("checkbuffer, bbbbbbbbbbbb\n");
                BbfDataBufferLen = frmNum * bbfRow;
		if(bbfData != NULL)
			free(bbfData);
		bbfData = (float **)malloc(BbfDataBufferLen * sizeof(float));
		for(int i=0; i<BbfDataBufferLen; i++)
		    bbfData[i] = (float*)malloc(bbfCol * sizeof(float));
		//printf("checkbuffer, bbbbbbbbbbbb  end, BbfDataBufferLen=%d\n", BbfDataBufferLen);
                //R2_DEL_AR2(this->bbfData);
                //this->bbfData = R2_NEW_AR2(float, BbfDataBufferLen, bbfCol);
        }

        if (frmNum > SlDataBufferLen) {
		//printf("checkbuffer, cccccccccccccc\n");
                SlDataBufferLen = frmNum;
		if(slData != NULL)
			free(slData);
		slData = (float **)malloc(SlDataBufferLen * sizeof(float));
		for(int i=0; i<SlDataBufferLen; i++)
		    slData[i] = (float*)malloc(slCol * sizeof(float));
		//printf("checkbuffer, cccccccccccccc   end\n");
                //R2_DEL_AR2(this->slData);
                //this->slData = R2_NEW_AR2(float, SlDataBufferLen, slCol);
        }
}

