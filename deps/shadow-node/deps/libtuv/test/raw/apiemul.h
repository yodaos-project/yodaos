/* Copyright 2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __apiemul_header__
#define __apiemul_header__

#ifdef __cplusplus
extern "C" {
#endif


#define APIEMUL_DUMP_HEAP_ADDRESS 1


int apiemultester_entry(void);

void apiemul_main(void);
void apiemul_socket(void);


#ifdef __cplusplus
}
#endif

#endif
