/*
 * micro-tpm test program (utpmtest)
 * author: amit vasudevan (amitvasudevan@acm.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <uhcall.h>
#include <xmhfcrypto.h>
#include <utpm.h>
#include <utpmtest.h>
//__attribute__((aligned(4096))) static uhcalltest_param_t uhctp;

#define _XDPRINTF_ 	printf

#if 0

void utpm_test(uint32_t cpuid)
	//////
	// utpm test
	//////
	{
		utpm_master_state_t utpm;
		TPM_DIGEST pcr0;
		TPM_DIGEST measurement;
		uint8_t digest[] =
				{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
				};

		uint8_t g_aeskey[TPM_AES_KEY_LEN_BYTES] =
				{
						0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
						0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
				};

		uint8_t g_hmackey[TPM_HMAC_KEY_LEN] =
				{
						0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x02,
						0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x11, 0x12,
						0xaa, 0xbb, 0xcc, 0xdd
				};

		uint8_t g_rsakey[] = {0x00, 0x00, 0x00, 0x00};
		char *seal_inbuf = "0123456789abcdef";
		uint32_t seal_inbuf_len = strlen(seal_inbuf)+1;
		char seal_outbuf[32];
		char seal_outbuf2[32];
		uint32_t seal_outbuf_len;
		uint32_t seal_outbuf2_len;
		TPM_PCR_INFO tpmPcrInfo;
		TPM_COMPOSITE_HASH digestAtCreation;

		if (utpm_init_master_entropy(&g_aeskey, &g_hmackey, &g_rsakey) != UTPM_SUCCESS){
			_XDPRINTF_("%s[%u]: utpm_init_master_entropy FAILED. Halting!\n", __func__, cpuid);
			exit(1);
		}

		utpm_init_instance(&utpm);

		if( utpm_pcrread(&pcr0, &utpm, 0) != UTPM_SUCCESS ){
			_XDPRINTF_("%s[%u]: utpm_pcrread FAILED. Halting!\n", __func__, cpuid);
			exit(1);
		}

		_XDPRINTF_("%s[%u]: pcr-0: %20D\n", __func__, cpuid, pcr0.value, " ");

		memcpy(&measurement.value, &digest, sizeof(digest));

		if( utpm_extend(&measurement, &utpm, 0) != UTPM_SUCCESS ){
			_XDPRINTF_("%s[%u]: utpm_extend FAILED. Halting!\n", __func__, cpuid);
			exit(1);
		}

		if( utpm_pcrread(&pcr0, &utpm, 0) != UTPM_SUCCESS ){
			_XDPRINTF_("%s[%u]: utpm_pcrread FAILED. Halting!\n", __func__, cpuid);
			exit(1);
		}

		_XDPRINTF_("%s[%u]: pcr-0: %20D\n", __func__, cpuid, pcr0.value, " ");

		tpmPcrInfo.pcrSelection.sizeOfSelect = 0;
		tpmPcrInfo.pcrSelection.pcrSelect[0] = 0;

		if( utpm_seal(&utpm, &tpmPcrInfo,
		                     seal_inbuf, seal_inbuf_len,
		                     seal_outbuf, &seal_outbuf_len) ){
			_XDPRINTF_("%s[%u]: utpm_seal FAILED. Halting!\n", __func__, cpuid);
			exit(1);
		}

		_XDPRINTF_("%s[%u]: utpm_seal PASSED\n", __func__, cpuid);

		if( utpm_unseal(&utpm,
		                       seal_outbuf, seal_outbuf_len,
		                       seal_outbuf2, &seal_outbuf2_len,
		                       &digestAtCreation) ){
			_XDPRINTF_("%s[%u]: utpm_unseal FAILED. Halting!\n", __func__, cpuid);
			exit(1);
		}

		_XDPRINTF_("%s[%u]: utpm_unseal PASSED\n", __func__, cpuid);

	}
#endif //0


uint8_t g_aeskey[TPM_AES_KEY_LEN_BYTES] =
		{
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
				0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
		};

uint8_t g_hmackey[TPM_HMAC_KEY_LEN] =
		{
				0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x02,
				0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x11, 0x12,
				0xaa, 0xbb, 0xcc, 0xdd
		};

uint8_t g_rsakey[] = {0x00, 0x00, 0x00, 0x00};

uint8_t digest[] =
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
		};


char *seal_inbuf = "0123456789abcdef";



__attribute__((aligned(4096))) utpmtest_param_t utpmtest_param;


//////
// utpm test
//////
void utpm_test(uint32_t cpuid)
{
	//sanity check
	if( sizeof(utpmtest_param_t) > 4096){
		_XDPRINTF_("%s[%u]: utpm_test: utpmtest_param_t > 4096. Halting!\n", __func__, cpuid);
		exit(1);
	}


	memcpy(&utpmtest_param.g_aeskey, &g_aeskey, TPM_AES_KEY_LEN_BYTES);
	memcpy(&utpmtest_param.g_hmackey, &g_hmackey, TPM_HMAC_KEY_LEN);
	memcpy(&utpmtest_param.g_rsakey, &g_rsakey, 4); //TODO: change to RSA key len when implemented

	if(!uhcall(UAPP_UTPM_FUNCTION_INIT_MASTER_ENTROPY, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_init_master_entropy hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}

	if (utpmtest_param.result != UTPM_SUCCESS){
		_XDPRINTF_("%s[%u]: utpm_init_master_entropy FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}

	if(!uhcall(UAPP_UTPM_FUNCTION_INIT_INSTANCE, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_init_instance hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}

	utpmtest_param.pcr_num=0;
	if(!uhcall(UAPP_UTPM_FUNCTION_PCRREAD, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_pcrread hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}
	if (utpmtest_param.result != UTPM_SUCCESS){
		_XDPRINTF_("%s[%u]: utpm_pcrread FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}

	_XDPRINTF_("%s[%u]: pcr-0: %20D\n", __func__, cpuid, utpmtest_param.pcr0.value, " ");

	memcpy(&utpmtest_param.measurement.value, &digest, sizeof(digest));
	utpmtest_param.pcr_num = 0;

	if(!uhcall(UAPP_UTPM_FUNCTION_EXTEND, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_extend hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}
	if (utpmtest_param.result != UTPM_SUCCESS){
		_XDPRINTF_("%s[%u]: utpm_extend FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}


	utpmtest_param.pcr_num=0;
	if(!uhcall(UAPP_UTPM_FUNCTION_PCRREAD, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_pcrread hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}
	if (utpmtest_param.result != UTPM_SUCCESS){
		_XDPRINTF_("%s[%u]: utpm_pcrread FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}

	_XDPRINTF_("%s[%u]: pcr-0: %20D\n", __func__, cpuid, utpmtest_param.pcr0.value, " ");


#if 0
	utpmtest_param.tpmPcrInfo.pcrSelection.sizeOfSelect = 0;
	utpmtest_param.tpmPcrInfo.pcrSelection.pcrSelect[0] = 0;
	memcpy(utpmtest_param.seal_inbuf, seal_inbuf, 16);
	utpmtest_param.seal_inbuf_len = 16;

	if(!uhcall(UAPP_UTPM_FUNCTION_SEAL, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_seal hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}
	if (utpmtest_param.result != UTPM_SUCCESS){
		_XDPRINTF_("%s[%u]: utpm_seal FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}


	_XDPRINTF_("%s[%u]: utpm_seal PASSED\n", __func__, cpuid);

	if(!uhcall(UAPP_UTPM_FUNCTION_UNSEAL, &utpmtest_param, sizeof(utpmtest_param_t))){
		_XDPRINTF_("%s[%u]: utpm_unseal hypercall FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}
	if (utpmtest_param.result != UTPM_SUCCESS){
		_XDPRINTF_("%s[%u]: utpm_unseal FAILED. Halting!\n", __func__, cpuid);
		exit(1);
	}

	_XDPRINTF_("%s[%u]: utpm_unseal PASSED\n", __func__, cpuid);
#endif

}



int main(){

   printf("Starting usr mode utpm test...\n");

   utpm_test(0);

   printf("End of utpm test\n");
   return 0;
}
