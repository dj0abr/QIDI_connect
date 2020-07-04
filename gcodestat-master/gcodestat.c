/*
 * File gcodestat.c v0.9
 * Author: arhimed@gmail.com
 * Date 20070721
 * 
 * modified by dj0abr@darc.de 
 * Date 20200704
 *
 */

#define _CLEANUP_ {if (gcodefile != NULL) free(gcodefile); \
                   if (gcodeout != NULL) free(gcodeout); \
                   if (apiurl != NULL) free(apiurl); \
                   if (apikey!=NULL) free(apikey); \
                   if (m117format!=NULL) free(m117format); \
                  }

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <malloc.h>
#include <sys/time.h>

#include "gcodestat.h"
#include "calcmove.h"
#include "readconfig.h"
#include "readgcode.h"

/*
 * print_config(*print_settings) - print config info
 */
void print_config(print_settings_t * ps) {
   /*fprintf(stderr, "gcodestat v0.9\n");
   fprintf(stderr, "Starting with parameters:\n");
   fprintf(stderr, "\tacceleration: \t\t%f mm/sec/sec\n", ps->accel);
   fprintf(stderr, "\tjunction deviation: \t%f\n", ps->jdev);
   fprintf(stderr, "\tx max speed: \t\t%f mm/min\n", ps->x_maxspeed);
   fprintf(stderr, "\ty max speed: \t\t%f mm/min\n", ps->y_maxspeed);
   fprintf(stderr, "\tz max speed: \t\t%f mm/min\n", ps->z_maxspeed);
   fprintf(stderr, "\tretraction time: \t%f sec\n", ps->rtime);
   fprintf(stderr, "\tprime time: \t\t%f sec\n", ps->ptime);
   fprintf(stderr, "\tspeed override: \t%f\n", ps->speedoverride);
   fprintf(stderr, "\tabsolute movement: \t%s\n", ps->abs ? "yes" : "no");
   fprintf(stderr, "\tabsolute extrusion: \t%s\n", ps->eabs ? "yes" : "no");
   fprintf(stderr, "\tmetering unit: \t\t%s\n", ps->mm ? "mm" : "inch");
   fprintf(stderr, "\n");*/
   return;
}

// copied function somewhere from the vast internet
char *str_replace(char *orig, char *rep, char *with) {
    char *result, *ins, *tmp;
    int len_rep, len_with, len_front, count;

    if (!orig || !rep) return NULL;
    if ((len_rep = strlen(rep)) == 0)  return NULL;
    if (!with) with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) ins = tmp + len_rep;
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result) return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}

void print_timeleft(FILE* f, long int sec){
    fprintf(f, "( ");
    if (sec > 60*60*24*7) fprintf(f, "%ld weeks, ", sec / 60 / 60 / 24 / 7);
    if (sec > 60*60*24  ) fprintf(f, "%ld days, " , (sec / 60 / 60 / 24) % 7);
    if (sec > 60*60     ) fprintf(f, "%02ld:"     , (sec / 3600) %24);
    fprintf(f, "%02ld:"                           , (sec / 60) % 60);
    fprintf(f, "%02ld )"                          ,  sec % 60);
}

void print_timeleft_f(FILE* f, char * sformat, long int sec, int pct){
  int week, day, hour, minute, second;
  char sweek[4], sday[2], shour[3], sminute[3], ssecond[3], sSec[64], spct[4];
  char *stringtowrite = NULL;
  char *tmp;

  if (sformat == NULL) return;
  if (pct == 0) return;
  if (sec == 0) return;

  week =    sec / 60 / 60 / 24 / 7;   // %w
  day  =   (sec / 60 / 60 / 24) % 7;  // %d
  hour =   (sec / 3600) %24;          // %h
  minute = (sec / 60) % 60;           // %m
  second =  sec % 60;                 // %s

  snprintf(sweek,   4, "%i", week);
  snprintf(sday,    2, "%i", day );
  snprintf(shour,   3, "%02i", hour);
  snprintf(sminute, 3, "%02i", minute);
  snprintf(ssecond, 3, "%02i", second);
  snprintf(sSec,   64, "%lu", sec);
  snprintf(spct,    4, "%i", pct);

  tmp = str_replace(sformat, "%w", sweek );
  if (tmp != NULL) {
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%d", sday );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%h", shour );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%m", sminute );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%s", ssecond );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%S", sSec );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%q", "\"" );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%%", "%" );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }
  tmp = str_replace(stringtowrite, "%p", spct );
  if (tmp != NULL) {
     free(stringtowrite);
     stringtowrite = strdup (tmp);
     free(tmp);
  }

  if (stringtowrite != NULL) {
     fputs(stringtowrite, f);
     fputs("\n", f);
     free(stringtowrite);
  }
}

#ifndef NOCURL
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
   size_t realsize = size * nmemb;
   return realsize;
}
#endif

/*
 * main
 */
int main(int argc, char** argv) {
   FILE *f = NULL;
   FILE *output_file = NULL;
   char *lb = NULL;
   char *gcodefile = NULL;
   char *gcodeout = NULL;
   char *m117format = NULL;
   double seconds = 0;
   double total_seconds = 0;
   print_settings_t print_settings;
   bool quiet = false;
   int pass;
   double next_pct = 100;
   double pct_step = 0.1;
   double heatup_time = 0.0;
   char *apikey = NULL;
   char *apiurl = NULL;
#ifndef NOCURL
   CURL *curl = NULL;
   CURLcode res;
   curl_mime *form = NULL;
   curl_mimepart *field = NULL;
   struct curl_slist *headerlist = NULL;
   void *chunk = NULL;
#endif

#ifdef _WIN32
   int alert = 0;
#endif

   static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"quiet", no_argument, NULL, 'q'},
      {"alert", no_argument, NULL, 'w'},
	   {"gcode", required_argument, NULL, 'g'},
      {"output", required_argument, NULL, 'o'},
      {"config", required_argument, NULL, 'c'},
      {"acceleration", required_argument, NULL, 'a'},
      {"junction_deviation", required_argument, NULL, 'd'},
      {"jerk", required_argument, NULL, 'j'},
      {"heatup_time", required_argument, NULL, 't'},
      {"max_x_speed", required_argument, NULL, 'x'},
      {"max_y_speed", required_argument, NULL, 'y'},
      {"max_z_speed", required_argument, NULL, 'z'},
      {"max_feed", required_argument, NULL, 'f'},
      {"retract_time", required_argument, NULL, 'r'},
      {"prime_time", required_argument, NULL, 'p'},
      {"percent_step", required_argument, NULL, 's'},
      {"api_url", required_argument, NULL, 'u'},
      {"api_key", required_argument, NULL, 'k'},
      {"m117_format", required_argument, NULL, 'm'},
      {NULL, 0, NULL, 0}
   };


   print_settings.accel         = DEFAULT_ACCELERATION;        // default acceleration mm/sec/sec
   print_settings.jdev          = DEFAULT_JUNCTION_DEVIATION;  // default junction deviation (ex jerk)
   print_settings.abs           = true;                        // by default ABS moves
   print_settings.eabs          = true;                        // by default extruder movement is absolute
   print_settings.mm            = true;                        // by default move in mm
   print_settings.x_maxspeed    = DEFAULT_MAX_SPEED;           // default max speed
   print_settings.y_maxspeed    = DEFAULT_MAX_SPEED;           // default max speed
   print_settings.z_maxspeed    = DEFAULT_MAX_SPEED;           // default max speed
   print_settings.rtime         = DEFAULT_RETRACT_TIME;        // default retract time
   print_settings.ptime         = DEFAULT_PRIME_TIME;          // default prime time
   print_settings.jerk          = false;                       // default we use junction deviation
   print_settings.speedoverride = 1.0;
   quiet                        = false;
   next_pct                     = 0.9;
   heatup_time                  = 0.0;
   seconds                      = heatup_time;
   apikey                       = NULL;
   apiurl                       = NULL;
  
   int option_index = 0;
	int getopt_result;
	while ((getopt_result = getopt_long(argc, argv, "q?hwg:c:a:d:j:x:y:z:r:p:o:s:t:u:k:m:", long_options, &option_index)) != -1) {
		switch (getopt_result) {
		case 'q':
			quiet = true;
			break;
		case 'h':
      case 'm':
         m117format = strdup(optarg);
         break;
		case 'g':
			if (optarg) {
				gcodefile = strdup(optarg);
			} 
			break;
		case 'o':
			if (optarg) {
				gcodeout = strdup(optarg);
			}
			break;
		case 'c':
			if (optarg) {
				if (read_config(optarg, &print_settings)) {
					fprintf(stderr, "Error reading config from %s\n", optarg);
					_CLEANUP_
					return (-1);
				}
			}
			break;
		case 'a':
			if (optarg) {
				print_settings.accel = atof(optarg);
				if (print_settings.accel == 0) {
					fprintf(stderr, "Invalid acceleration value: %s\n", optarg);
					_CLEANUP_
					return (-1);
				}
			}
			break;
		case 'd':
			if (optarg) {
				print_settings.jdev = atof(optarg);
				if (print_settings.jdev == 0) {
					fprintf(stderr, "Invalid junction deviation value: %s\n",
							optarg);
					_CLEANUP_
					return (-1);
				}
			}
			break;
		case 'j':
			if (optarg) {
				//TODO: implement jerk thingy
				fprintf(stderr, "TODO: JERK IS NOT YET SUPORTED, SORRY\n");
				return (-999);
			}
			break;
		case 't':
			if (optarg) {
				heatup_time = atof(optarg);
			}
			break;
		case 'x':
			if (optarg) {
				print_settings.x_maxspeed = atof(optarg);
				if (print_settings.x_maxspeed == 0) {
					fprintf(stderr, "Invalid max_x_speed value: %s\n", optarg);
					return (-1);
				}
			} 
			break;
		case 'y':
			if (optarg) {
				print_settings.y_maxspeed = atof(optarg);
				if (print_settings.y_maxspeed == 0) {
					fprintf(stderr, "Invalid max_y_speed value: %s\n", optarg);
					_CLEANUP_
					return (-1);
				}
			} 
			break;
		case 'z':
			if (optarg) {
				print_settings.z_maxspeed = atof(optarg);
				if (print_settings.z_maxspeed == 0) {
					fprintf(stderr, "Invalid max_z_speed value: %s\n", optarg);
					_CLEANUP_
					return (-1);
				}
			} 
			break;
		case 'r':
			if (optarg) {
				print_settings.rtime = atof(optarg);
				if (print_settings.rtime == 0) {
					fprintf(stderr, "Invalid retraction time value: %s\n",
							optarg);
					_CLEANUP_
					return (-1);
				}
			} 
			break;
		case 'p':
			if (optarg) {
				print_settings.ptime = atof(optarg);
				if (print_settings.ptime == 0) {
					fprintf(stderr, "Invalid prime time value: %s\n", optarg);
					_CLEANUP_
					return (-1);
				}
			} 
			break;
		case 's':
			if (optarg) {
				pct_step = atof(optarg)/100.0F;
				if (pct_step > 1.0F || pct_step < 0.01F) {
					fprintf(stderr, "Invalid percent step value: %s, please use value between 1 and 100\n", optarg);
					_CLEANUP_
					return (-1);
				}
			} 
			break;
		case 'u':
			if (optarg) {
				apiurl = strdup(optarg);
			} 
			break;
		case 'k':
			if (optarg) {
				apikey = strdup(optarg);
			} 
			break;

		}
	}

	if (m117format == NULL) m117format = strdup("M117 %p%% Remaining %w weeks %d days ( %h:%m:%s )");
   if (gcodefile == NULL)
      gcodefile = strdup("samples/test.gcode");

   if (NULL == (lb = calloc(LINE_BUFFER_LENGTH, 1))) {
      fprintf(stderr, "ERROR: Cannot allocate ram for line buffer\n");
      _CLEANUP_
      return (-3);
   }

   if (NULL == (f = fopen(gcodefile, "r"))) {
      fprintf(stderr, "Cannot open %s for read\n", gcodefile);
      
      _CLEANUP_
      return (-2);
   }

   if (gcodeout != NULL) {
      if (NULL == (output_file = fopen(gcodeout, "w"))) {
         fprintf(stderr, "Cannot open %s for write\n", gcodeout);
         fclose(f);
      
         _CLEANUP_
         return (-2);
      }
   }

   if (!quiet)
      print_config(&print_settings);
   next_pct = 1 - pct_step;
   for (pass = 0; pass < (output_file != NULL ? 2 : 1); pass++) {
      if (pass == 1) {
         rewind(f);
         total_seconds = seconds;
         seconds = heatup_time;
         //fprintf(output_file, "M117 100%% Remaining ");
         //print_timeleft(output_file, (long int) floor(total_seconds));
         //fprintf(output_file, "\n");
         print_timeleft_f(output_file, m117format, (long int) floor(total_seconds), 100);

      }
      while (fgets(lb, LINE_BUFFER_LENGTH, f)) {
         if (pass == 1) {
            fputs(lb, output_file);
            if (next_pct > 0.01 && (total_seconds - seconds) / total_seconds < next_pct) {
               //fprintf(output_file, "M117 %i%% Remaining ", (int) floor(next_pct * 100));
               //print_timeleft(output_file, (long int) floor(total_seconds - seconds));
               //fprintf(output_file, "\n");
               print_timeleft_f(output_file, m117format, (long int) floor(total_seconds - seconds), (int) floor(next_pct * 100));
               next_pct -= pct_step;
            }
         }

         if (comment(lb))
            continue;

         switch (gcode(lb)) {
         case GCODE_MOVE:
            seconds += calcmove(lb, &print_settings);
            break;
         case GCODE_DWELL:
            seconds += read_dwell(lb);
            break;
         case GCODE_RETRACT:
            break;
         case GCODE_INCH:
            print_settings.mm = false;
            break;
         case GCODE_MM:
            print_settings.mm = true;
            break;
         case GCODE_ABS:
            print_settings.abs = true;
            break;
         case GCODE_REL:
            print_settings.abs = false;
            break;
         case GCODE_EABS:
            print_settings.eabs = true;
            break;
         case GCODE_EREL:
            print_settings.eabs = false;
            break;
         case GCODE_MAXFEED:
            read_maxfeed(lb, &print_settings);
            break;
         case GCODE_ACCEL:
            read_accel(lb, &print_settings);
            break;
         case GCODE_JDEV:
            read_jdev(lb, &print_settings);
            break;
         case GCODE_RLEN:
            read_rtime(lb, &print_settings);
            break;
         case GCODE_PLEN:
            read_ptime(lb, &print_settings);
            break;
         case GCODE_SPEEDOVER:
            read_speedover(lb, &print_settings);
            break;
         }

      }
   }

   fclose(f);
   
   printf("%ld\n",(unsigned long)seconds);

   _CLEANUP_
   return 0;
}
