/*** DES-PWE license ***

   Copyright (c) 2019 by itc

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   DES-PWE migration (c) 2023 by itc
   current version：v1.0
   License Agreement： apache license 2.0

   

   云版本为开源版提供永久免费提示通道支持，提示通道支持短信、邮件、
   微信等多种方式，欢迎使用

   模块 slog_mtreport_client 功能:
        用于上报除迁移云管系统本身产生的监控点数据、日志，为减少部署上的依赖
		未引入任何第三方组件

****/

/*** DES-PWE license ***

   Copyright (c) 2019 by itc

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   DES-PWE migration (c) 2023 by itc
   current version：v1.0
   License Agreement： apache license 2.0

   

   云版本为开源版提供永久免费提示通道支持，提示通道支持短信、邮件、
   微信等多种方式，欢迎使用

   开发库 mtagent_api_open 说明:
        itc云框架系统内部公共库，提供各种公共的基础函数调用

   说明：aes.h/aes.cpp 来源于网络

****/

#ifndef _AES_H_
#define _AES_H_

/* key length */
#define AES_128	16
#define AES_192	24
#define AES_256	32

typedef unsigned char	uint8_t;

/**
 * @param in - source data.
 * @param in_len - source data length.
 * @param out - ouput, because of padding, the length must be equal to ((in_len>>4)+1)<<4.
 * @param key - the length of the key must be equal to AES_128(16) or AES_192(24) or AES_256(32)
 * @param key_len - AES_128 or AES_192 or AES_256
 */
void aes_cipher_data(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len);

/**
 * @param in - source data.
 * @param in_len - source data length, and it must be divisible by 16
 * @param out - ouput, the size must at least be equal to in_len.
 * @param out_len - it's a reference and less than in_len because of padding
 * @param key - the length of the key must be equal to AES_128(16) or AES_192(24) or AES_256(32)
 * @param key_len - AES_128 or AES_192 or AES_256
 */
void aes_decipher_data(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len, const uint8_t *key, size_t key_len);

/**
 * cipher file, not remove source file.
 * @param in_filename - source filename.
 * @param out_filename - destination filename. because of padding , the file size will be divisible by 16.
 * @param key - the length of the key must be equal to AES_128(16) or AES_192(24) or AES_256(32)
 * @param key_len - AES_128 or AES_192 or AES_256
 * @return 0-sucessful,1-failed
 */
int aes_cipher_file(const char *in_filename, const char *out_filename, const uint8_t *key, size_t key_len);

/**
 * decipher file, not remove source file.
 * @param in_filename - source filename and file size must be divisible by 16.
 * @param out_filename - destination filename
 * @param key - the length of the key must be equal to AES_128(16) or AES_192(24) or AES_256(32)
 * @param key_len - AES_128 or AES_192 or AES_256
 * @return 0-sucessful,1-failed
 */
int aes_decipher_file(const char *in_filename, const char *out_filename, const uint8_t *key, size_t key_len);

#endif /* !_AES_H_ */

