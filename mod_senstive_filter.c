/* Licensed to the Apache Software Foundation (ASF) under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The ASF licenses this file to You under the Apache License, Version 2.0
* (the "License"); you may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include "httpd.h"
#include "http_config.h"
#include "apr_buckets.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_file_io.h"
#include "util_filter.h"
#include "http_request.h"
#include "stdio.h"

#define senbytes 60		//define the bytes of the sentive word
#define dirbytes 30 		//define the bytes of the dir

static const char s_szSenstiveFilterName[]="SenstiveFilter";
module AP_MODULE_DECLARE_DATA senstive_filter_module;

typedef struct
{
	int bEnabled;
} SenstiveFilterConfig;

//store the senstive words from the file
typedef struct 
{
	char word[senbytes];
	int len;	
} SenWord;

//store the directories
typedef struct
{
	char dirs[dirbytes];
	int len;
} Dirs;

static void *SenstiveFilterCreateServerConfig(apr_pool_t *p,server_rec *s)
{
	SenstiveFilterConfig *pConfig=apr_pcalloc(p,sizeof *pConfig);
	pConfig->bEnabled=0;

	return pConfig;
}

static void SenstiveFilterInsertFilter(request_rec *r)
{
	SenstiveFilterConfig *pConfig=ap_get_module_config(r->server->module_config,&senstive_filter_module);

	if(!pConfig->bEnabled)
		return;

	ap_add_output_filter(s_szSenstiveFilterName,NULL,r,r->connection);
}


//Return the sign of charset 
//1 = GB2312 ,2 = Unicode , 3 = UTF-8
//char * r->content_type must the content_type in the struct request *rec  
int Encode(char * content_type)
{
	char * utf = "text/html;charset=utf-8";
	char * gb = "text/html;charset=gb2312";
	char * uni = "text/html;charset=unicode";
	
	if( apr_strnatcasecmp(content_type,utf) == 0 )		//apr_strnatcasecmp can compare the string without the case
	{
		return 3;
	}

	else if ( apr_strnatcasecmp(content_type,gb) == 0 )
	{
		return 1;
	}
	
	else if( apr_strnatcasecmp(content_type,uni) == 0 )
	{
		return 2;
	}
}

//GetSenWord is the function to get the senstive words from the file
//GetResource is the function to get the Resouces from files
//then return the point of the source's address and amount .(use the struct SenWord or Dirs)
//choice 1 for words get,2 for dirs get
int GetResource(SenWord *senword,Dirs * dir,int choice)
{
	int amount = 0;
	int i = 0;
	int Lineflag = 1,readeffect = 1; 
	apr_pool_t *pool;
	apr_file_t *fd = NULL;

	apr_pool_create(&pool,NULL);

	char *ch = apr_pcalloc(pool,sizeof *ch);
	
	if(choice == 1)
	{
		apr_file_open(&fd,"/etc/apache2/sen.txt",APR_READ,0,pool);	//The senstive file name is define here
	}
	else
	{
		apr_file_open(&fd,"/etc/apache2/dir.txt",APR_READ,0,pool);	//The Directories' file is define here
	}

	apr_file_getc(ch,fd);

	if( apr_file_eof(fd) == APR_EOF )
		amount = 0;	
	else
	{
		if( (int)choice == 1 )
		{
			while( apr_file_eof(fd) != APR_EOF )
			{
					if( (unsigned int)ch[0] != ',' )
					{
						if(amount == 0)
							amount = 2;
						senword[(int)amount].word[i] = ch[0];
						i++;
					}
					else
					{
						senword[(int)amount].len = i;
						amount ++;
						
						i = 0;
					}
				apr_file_getc(ch,fd);
			}
		}
		else
		{
			while( apr_file_eof(fd) != APR_EOF )
			{
				if( (unsigned int)ch[0] != 10 )
				{
					dir[(int)amount].dirs[i] = ch[0];
					i++;
				}
				else
				{
					dir[(int)amount].len = i;
					amount ++;
					i = 0;
				}
				apr_file_getc(ch,fd);
			}
		}

	}
	
	apr_file_close(fd);

	if(amount != 0)
		//subtract 1 which added by the end of the 0x0A
		amount = amount - 1;	

	return amount;	
}

//SeaSen is short for search senstive word
//this function is used for searching senstive word from the data[n]
//and change them ,then copy to the buf which will send to the clien
//Encode 1 = GB2312 ,2 = Unicode , 3 = UTF-8
char * SeaSen(const char * data,apr_size_t len,int checkflag,apr_pool_t *pool ,apr_bucket_alloc_t *parent)
{

	int Dn,Ln,An,flag = 0,amount_word;

	SenWord *senword = apr_pcalloc(pool,sizeof *senword); 	//malloc a memory to the struct

	amount_word = GetResource(senword,NULL,1);			//Use function to full fill the struct

	char *buf = apr_bucket_alloc(len,parent);

	if(checkflag == 1)
	{
		//Data recycle
		for(Dn = 0 ; Dn < len ; Dn++)
		{
			//The senwords' amount recycle
			//Attention due to the paclloc bug I should make an = 2 with 1 as in the GetResource();
			for( An = 2; An <= amount_word ; An ++)
			{
				//The length of the sensword recycle
				for( Ln = 0 ; Ln < senword[An].len ; Ln ++) 
				{
					if( data[Dn+Ln] == senword[An].word[Ln])
						flag ++ ;
					else
						break;
				}
	

				// check the flag which means the senstive word is finded or not
				if( flag == senword[An].len )
				{
					for( Ln = 0 ; Ln < senword[An].len ; Ln ++)
					{
						//use '*' to change the senstive word
						buf[Ln+Dn] = '*';
					}
	
					//Jump the characters which had been changed
					Dn = Dn + senword[An].len ;
				}
	
				flag = 0;
			}


		//If not find the character , so the character data[Dn] must use the original one
			buf[Dn] = data[Dn];
		}
	}
	else
	{
		for(Dn = 0 ; Dn < len ; ++Dn)
			buf[Dn] = data[Dn];
	}


	return buf;
}



//Check the Directory
//Check the directory is in the file or not
//return flase = 0 ,true = 1
int CheckDir(char * path,apr_pool_t *parent)//,apr_pool_t *parent)//,int amount)
{
	int flag = 0;
	int n = 1,i,amount;
	apr_pool_t *pool;
	apr_pool_create(&pool,parent);
	
	Dirs *dir_data = apr_palloc(pool,sizeof *dir_data);
	char *path_current = path;//apr_palloc()
	
	amount = GetResource(NULL,dir_data,2);

	//Attention bug is here
	for(n = 1 ;n <= amount; n++,flag = 0)
	{
		//copy dir_data[amount]'s length from path_current to data
		//apr_cpystrn(data_com,path_current,sizeof(dir_data[amount].dirs[(dir_data[amount].len)+1]));
	
		for(i = 0;i < dir_data[n].len;i++)
		{
			if( dir_data[n].dirs[i] == path_current[i]  && path_current[i] )
			{
				flag ++ ;
			}
			else
				break;
		}

		if (flag == dir_data[n].len )
		{
			return 1;
		}
	}
	return 0;
}

static apr_status_t SenstiveFilterOutFilter(ap_filter_t *f,apr_bucket_brigade *pbbIn)
{
	//amount_dir,amount_word are the amount of the dirs and senstive words 
	//checkflag is the flag of the directory whether is in or not 
	int  amount_word, amount_dir;
	int checkflag = 0;
	int n,i;

	request_rec *r = f->r;

	//checkflag = Encode(r->content_type);

	//Attention below two lines had moved to the function SeaSen()
//	SenWord *senword_data = apr_pcalloc(r->pool,sizeof *senword_data); 	//malloc a memory to the struct

//	amount_word = GetResource(senword_data,NULL,1);			//Use function to full fill the struct
	
	checkflag = CheckDir(r->unparsed_uri,r->pool);


		conn_rec *c = r->connection;
		apr_bucket *pbktIn;
		apr_bucket_brigade *pbbOut;


		pbbOut=apr_brigade_create(r->pool, c->bucket_alloc);
		for (pbktIn = APR_BRIGADE_FIRST(pbbIn);
				pbktIn != APR_BRIGADE_SENTINEL(pbbIn);
				pbktIn = APR_BUCKET_NEXT(pbktIn))
		{
			const char *data;
			apr_size_t len;
			char *buf;
			apr_size_t n;
			apr_bucket *pbktOut;
		
			if(APR_BUCKET_IS_EOS(pbktIn))
			{
				apr_bucket *pbktEOS=apr_bucket_eos_create(c->bucket_alloc);
				APR_BRIGADE_INSERT_TAIL(pbbOut,pbktEOS);
				continue;
			}

			// read 
			apr_bucket_read(pbktIn,&data,&len,APR_BLOCK_READ);
		
			// write 
			buf = apr_bucket_alloc(len, c->bucket_alloc);
		
			// search the senstive word and change them
			buf = SeaSen(data,len,(int)checkflag,r->pool,c->bucket_alloc);//Encode(r->content_type));

			//below for test!
			/*
			for(n =0 ;n<len;n++)
			{
				if( checkflag == 1 )
					buf[n] = r->unparsed_uri[1];// data[n];
				else
					buf[n] = data[n];
			}
			*/			
			pbktOut = apr_bucket_heap_create(buf, len, apr_bucket_free,c->bucket_alloc);

			APR_BRIGADE_INSERT_TAIL(pbbOut,pbktOut);
		}

	return ap_pass_brigade(f->next,pbbOut);
	
}

static const char *SenstiveFilterEnable(cmd_parms *cmd, void *dummy, int arg)
{
	SenstiveFilterConfig *pConfig=ap_get_module_config(cmd->server->module_config,&senstive_filter_module);
	pConfig->bEnabled=arg;
	
	return NULL;
}

static const command_rec SenstiveFilterCmds[] =  {AP_INIT_FLAG("SenstiveFilter", SenstiveFilterEnable, NULL, RSRC_CONF,"Run the Senstive filter on this host"),{ NULL }};

static void SenstiveFilterRegisterHooks(apr_pool_t *p)
{
	// insert filter
	ap_hook_insert_filter(SenstiveFilterInsertFilter,NULL,NULL,APR_HOOK_MIDDLE);
	// regeister filter
	ap_register_output_filter(s_szSenstiveFilterName,SenstiveFilterOutFilter,NULL,AP_FTYPE_RESOURCE);
}

module AP_MODULE_DECLARE_DATA senstive_filter_module = {STANDARD20_MODULE_STUFF,NULL,NULL,SenstiveFilterCreateServerConfig,NULL,SenstiveFilterCmds,SenstiveFilterRegisterHooks};
