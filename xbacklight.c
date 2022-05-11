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


const char* APP_DESC = "control backlight brightness";
const char* backlight_path = "/sys/class/backlight";
const char* leds_path = "/sys/class/leds";
char* backlight_brightness=NULL;
char* backlight_max_brightness=NULL;
int DEBUG=false;
size_t strncmp_limit=20;
int max_brightness=0;
int brightness=0;
float f_brightness=0;

enum action{print,dec,inc,set};
enum action action=print;
int action_value=0;

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
	if( (backlight_brightness=malloc(512)) == NULL ){
		fprintf(stderr, "malloc error in %s:%d\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);}
	if( (backlight_max_brightness=malloc(512)) == NULL ){
		fprintf(stderr, "malloc error in %s:%d\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);}
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
br_inc(){
	int write_val=0;
	write_val=(int)(max_brightness * ((f_brightness+action_value)/100));
	if ( write_val > max_brightness ) write_val = max_brightness;
	if(DEBUG)fprintf(stderr,"%d\n", write_val );
	if (intToFile(backlight_brightness, write_val ))
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);}
void
br_set(){
	int write_val=0;
	write_val=(int)(max_brightness * ((float)action_value/100));
	if(write_val>max_brightness)write_val=max_brightness;
	if(write_val<0)write_val=0;
	if(DEBUG)fprintf(stderr,"%d\n", write_val );
	if (intToFile(backlight_brightness, write_val ))
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);}
void
br_dec(){
	int write_val=0;
	write_val=(int)(max_brightness * ((f_brightness-action_value)/100));
	if(write_val<0)write_val=0;
	if(DEBUG)fprintf(stderr,"%d\n", write_val );
	if (intToFile(backlight_brightness, write_val ))
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);}
	
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
	
	//
	// ARGS
	//
	
	// main parsing loop
	//for(;argc>1&&argv[1][0]=='-';argc--,argv++){
	for(;argc>1&&( argv[1][0]=='-' || argv[1][0]=='+' || argv[1][0]=='=' ) ;argc--,argv++){
		val=0;
		argPtr==NULL;
		argPtr=&argv[1][1];
		if(DEBUG) fprintf(stderr,"arg=\'%s\'\n",argPtr);
		//if(DEBUG) fprintf(stderr,"arg0=\'%s\'\n",&argv[1][0]);
		
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
		
		if(argv[1][1]=='h'){
			print_help();
			exit(EXIT_SUCCESS);}
		
		if(strncmp(argPtr, "d", strncmp_limit ) == 0){
			DEBUG=true;}
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
			return(EXIT_SUCCESS);
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
