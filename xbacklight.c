// xbacklight: control backlight and led brightness on linux using the sys
//             filesystem with a backward-compatibile user interface
// Copyright(c) 2016-2019 by wave++ "Yuri D'Elia" <wavexx@thregr.org>
// Copyright(c) 2022 adip "Adam Prycki" <adam.prycki@gmail.com>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>

#define _DEBUG 1

#if _DEBUG
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

void
print_trace(){
	void *array[10];
	char **strings;
	int size, i;
	
	size=backtrace (array, 10);
	strings = backtrace_symbols (array, size);
	if(strings){
		fprintf(stderr,"Obtained %d stack frames.\n", size);
		for (i = 0; i < size; i++)
			fprintf(stderr, "%s\n", strings[i]);}
	free (strings);}

#define null_check(X) if( (X) == NULL ){ \
		fprintf(stderr, "null pointer error in %s:%d\n",__FILE__,__LINE__); \
		print_trace(); \
		exit(EXIT_FAILURE);}

#else

#define null_check(X) 

#endif

#define DICT_START_SIZE 10
#define DICT_MAX_VAL_SIZE 511
#define DICT_MAX_KEY_SIZE 31
struct dict {
	char** key;
	char** val;
	int		count;
	int		size;
	};
typedef struct dict dict;

const char* APP_DESC = "control backlight brightness";
const char* backlight_path = "/sys/class/backlight";
const char* sys_paths[] = { "/sys/class/backlight", "/sys/class/leds" };
const char* leds_path = "/sys/class/leds";
dict* ctrl_d=NULL;
char backlight_brightness[512]={0};
char backlight_max_brightness[512]={0};
int DEBUG=0;
size_t strncmp_limit=20;
int max_brightness=0;
int brightness=0;
float f_brightness=0;

enum action{print,dec,inc,set};
enum action action=print;
int action_value=0;

dict*
dictInit(){
	dict* ret;
	ret=calloc(1,sizeof(dict));
	null_check(ret);
	ret->count=0;
	ret->size=DICT_START_SIZE;
	ret->key=calloc(DICT_START_SIZE, sizeof(char*));
	null_check(ret->key);
	ret->val=calloc(DICT_START_SIZE, sizeof(char*));
	null_check(ret->val);
	return(ret);}

void
dictExtend(dict* d){
	null_check(d);
	d->key = reallocarray( d->key, d->size+DICT_START_SIZE, sizeof(char*));  
	null_check(d->key);
	d->val = reallocarray( d->val, d->size+DICT_START_SIZE, sizeof(char*));  
	null_check(d->val);
	d->size+=DICT_START_SIZE;
	}

int
dictAppend(dict* d, const char* key, const char* val){
	int i=0;
	null_check(key);
	null_check(val);
	if(d->count+1 > d->size){
		dictExtend(d);}
	i=d->count++;
	d->key[i]=calloc( strnlen( key, DICT_MAX_KEY_SIZE )+1, sizeof(char));
	d->val[i]=calloc( strnlen( val, DICT_MAX_VAL_SIZE )+1, sizeof(char));
	null_check(d->key[i]);
	null_check(d->val[i]);
	strcpy(d->key[i], key);
	strcpy(d->val[i], val);
	return(0);}

void
dictFree(dict* d){
	null_check(d);
	for(int i=0;i<d->count;i++){
		free(d->key[i]);
		free(d->val[i]);}
	free(d->key);
	free(d->val);
	free(d);}

char*
dictGetVal(dict* d, char* key){
	printf("c=%d s=%d\n", d->count, d->size);
	for(int i=0;i<d->count;i++){
		//printf("k:%s v:%s\n", d->key[i], d->val[i]);
		//printf("i:%d k:%s v:%s\n", i, d->key[i], d->val[i]);
		if( strncmp( d->key[i], key, DICT_MAX_KEY_SIZE)==1 ){
			printf("i:%d k:%s v:%s\n", i, d->key[i], d->val[i]);
			return( d->val[i] );}}
	return(NULL);}

void
dictPrint(dict* d){
	fprintf(stderr, "__OBJ %p\n", d);
	fprintf(stderr, "c=%d s=%d\n", d->count, d->size);
	for(int i=0;i<d->count;i++){
		fprintf(stderr, "k:%s v:%s\n", d->key[i], d->val[i]);}
	fprintf(stderr, "__END_OF_OBJ %p\n", d);
		}

void
dictTest(){
	dict* d=dictInit();
	dictAppend(d, "0", "a");
	dictAppend(d, "1", "b");
	dictAppend(d, "2", "c");
	dictAppend(d, "3", "d");
	dictAppend(d, "4", "e");
	dictAppend(d, "5", "f");
	dictAppend(d, "6", "g");
	dictAppend(d, "7", "h");
	dictAppend(d, "8", "i");
	dictAppend(d, "9", "j");
	dictAppend(d, "10", "k");
	printf("dictGetVal test k:0 = v:%s\n", dictGetVal(d, "0"));
	printf("dictGetVal test k:5 = v:%s\n", dictGetVal(d, "5"));
	printf("dictGetVal test k:9 = v:%s\n", dictGetVal(d, "9"));
	dictPrint(d);
	dictFree(d);
	}

void
scan_dirs(){
	DIR *d;
	struct dirent *dir;
	char path[512];
	ctrl_d = dictInit();
	d=opendir(sys_paths[0]);
	if(d){
		while((dir=readdir(d))!=NULL){
			if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ){
				continue;}
			snprintf(path, 512, "%s/%s", sys_paths[0], dir->d_name );
			dictAppend(ctrl_d, dir->d_name, path);
			}
		closedir(d);}
	d=opendir(sys_paths[1]);
	if(d){
		while((dir=readdir(d))!=NULL){
			if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ){
				continue;}
			snprintf(path, 512, "%s/%s", sys_paths[1], dir->d_name );
			dictAppend(ctrl_d, dir->d_name, path);
			}
		closedir(d);}
	}

void
print_ctrls(){
	for(int i=0;i<ctrl_d->count;i++){
		printf("%-20s \t(path= %s)\n", ctrl_d->key[i], ctrl_d->val[i]);}}
	

int
fileToint(const char* path){
	int i=0;
	FILE* file=fopen(path, "r");
	if(file){
		if(fscanf(file, "%d", &i)==1){
			fclose(file);
			return(i);}
		else{
			fclose(file);
			return(-1);}}
	return(-1);}

int
intToFile(const char* path, int i){
	FILE* f=fopen(path, "w");
	if(f){
		fprintf(f, "%d", i);
		fclose(f);
		return(0);}
	else{
		perror(path);}
		return(1);}

void
setPathsAndBrightness(){	
	DIR *d;
	struct dirent *dir;
	d=opendir(backlight_path);
	if(d){
		while((dir=readdir(d))!=NULL){
			if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ){
				continue;}
			snprintf(backlight_brightness, 512, "%s/%s/brightness", backlight_path, dir->d_name );
			snprintf(backlight_max_brightness, 512, "%s/%s/max_brightness", backlight_path, dir->d_name );
			if(DEBUG){
				fprintf(stderr, "backlight file=%s\n", backlight_brightness);
				fprintf(stderr, "max_backlight file=%s\n", backlight_max_brightness);}}
		closedir(d);}
	max_brightness=fileToint(backlight_max_brightness);
	brightness=fileToint(backlight_brightness);
	f_brightness=(float)brightness/max_brightness*100;}


void
exit_success(){
	dictFree(ctrl_d);
	exit(EXIT_SUCCESS);}

void
br_inc(){
	int write_val=0;
	write_val=(int)(max_brightness * ((f_brightness+action_value)/100));
	if ( write_val > max_brightness ) write_val = max_brightness;
	if(DEBUG)fprintf(stderr,"%d\n", write_val );
	if (intToFile(backlight_brightness, write_val ))
		exit(EXIT_FAILURE);
	exit_success();}
void
br_set(){
	int write_val=0;
	write_val=(int)(max_brightness * ((float)action_value/100));
	if(write_val>max_brightness)write_val=max_brightness;
	if(write_val<0)write_val=0;
	if(DEBUG)fprintf(stderr,"%d\n", write_val );
	if (intToFile(backlight_brightness, write_val ))
		exit(EXIT_FAILURE);
	exit_success();}
void
br_dec(){
	int write_val=0;
	write_val=(int)(max_brightness * ((f_brightness-action_value)/100));
	if(write_val<0)write_val=0;
	if(DEBUG)fprintf(stderr,"%d\n", write_val );
	if (intToFile(backlight_brightness, write_val ))
		exit(EXIT_FAILURE);
	exit_success();}
	
void
print_help(){
	char* help =
	"usage: xbacklight [options]"
	"  where options are:"
	"  -display <display> or -d <display>"
	"  -help"
	"  -version"
	"  -set <percentage> or = <percentage>"
	"  -inc <percentage> or + <percentage>"
	"  -dec <percentage> or - <percentage>"
	"  -get"
	"  -time <fade time in milliseconds>"
	"  -steps <number of steps in fade>";
	puts(help);}

int
main(int argc, char* argv[]){
	char* argPtr=NULL;
	int val=0;
	scan_dirs();
	
	//
	// ARGS
	//
	// early check for debug flag
	for(int i=0;i<argc;i++){
		if(strncmp(argv[i],"-d",strncmp_limit)==0){
			printf("DEBUG argv[%d]=%s\n",i,argv[i]);
			DEBUG=true;}}
	
	// main parsing loop
	//for(;argc>1&&argv[1][0]=='-';argc--,argv++){
	for(;argc>1&&( argv[1][0]=='-' || argv[1][0]=='+' || argv[1][0]=='=' ) ;argc--,argv++){
		val=0;
		argPtr==NULL;
		argPtr=&argv[1][1];
		if(DEBUG) fprintf(stderr,"arg=\'%s\'\n",argPtr);
		
		// parse -<int>
		if(argv[1][0]=='-'){
			if(sscanf(&argv[1][1], "%d", &action_value)==1){
				if(DEBUG) fprintf(stderr,"action_value %d\n", action_value);
				action=dec;}}
		
		// parse +<int>
		if(argv[1][0]=='+'){
			if(sscanf(&argv[1][1], "%d", &action_value)==1){
				if(DEBUG) fprintf(stderr,"action_value %d\n", action_value);
				action=inc;}}
		
		// parse =<int>
		if(argv[1][0]=='='){
			if(sscanf(&argv[1][1], "%d", &action_value)==1){
				if(DEBUG) fprintf(stderr,"action_value %d\n", action_value);
				action=set;}}
		
		// -h
		if(argv[1][1]=='h'){
			print_help();
			exit_success();}
		
		// -d
		if(argv[1][1]=='d'){
			DEBUG=true;}
		
		// -ctrl
		if(strncmp(argPtr, "list", strncmp_limit ) == 0){
			print_ctrls();
			exit_success();}
			
		
		// -dec
		if(strncmp(argPtr, "dec", strncmp_limit ) == 0){
			if(DEBUG) fprintf(stderr,"action= dec, argc=%d\n", argc);
			if(argc>2){
				if(sscanf(&argv[2][0], "%d", &action_value)==1){
					if(DEBUG) fprintf(stderr,"action_value %d\n", action_value);
					action=dec;}
				else{
					print_help;
				}}}
		
		// -inc
		if(strncmp(argPtr, "inc", strncmp_limit ) == 0){
			if(DEBUG) fprintf(stderr,"action= inc, argc=%d\n", argc);
			if(argc>2){
				if(sscanf(&argv[2][0], "%d", &action_value)==1){
					if(DEBUG) fprintf(stderr,"action_value %d\n", action_value);
					action=inc;}
				else{
					print_help;
				}}}
		
		// -set
		if(strncmp(argPtr, "set", strncmp_limit ) == 0){
			if(DEBUG) fprintf(stderr,"action= set, argc=%d\n", argc);
			if(argc>2){
				if(sscanf(&argv[2][0], "%d", &action_value)==1){
					if(DEBUG) fprintf(stderr,"action_value %d\n", action_value);
					action=set;}
				else{
					print_help; }}}
		}
	
	if(DEBUG) fprintf(stderr, "argc= %d\n", argc);
	for(int i=0;i<argc;i++){
		if(DEBUG) fprintf(stderr, "arg[%d]= \"%s\"\n", i, argv[i]);}
	
	//
	// ACTIONS
	//
	skip_arg_parse:
	
	setPathsAndBrightness();
	
	switch(action){
		case print:
			printf("%f", f_brightness);
			exit_success();
		case set:
			if(DEBUG)fprintf(stderr,"set\n");
			br_set();
		case inc:
			if(DEBUG)fprintf(stderr,"inc\n");
			br_inc();
		case dec:
			if(DEBUG)fprintf(stderr,"dec\n");
			br_dec();
	}
	return(0);}
