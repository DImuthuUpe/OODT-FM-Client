#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>
#include <limits.h>
#include <string.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

struct entry_s {
	char *key;
	char *value;
	struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s {
	int size;
	struct entry_s **table;	
};

typedef struct hashtable_s hashtable_t;


/* Create a new hashtable. */
hashtable_t *ht_create( int size ) {

	hashtable_t *hashtable = NULL;
	int i;

	if( size < 1 ) return NULL;

	/* Allocate the table itself. */
	if( ( hashtable = malloc( sizeof( hashtable_t ) ) ) == NULL ) {
		return NULL;
	}

	/* Allocate pointers to the head nodes. */
	if( ( hashtable->table = malloc( sizeof( entry_t * ) * size ) ) == NULL ) {
		return NULL;
	}
	for( i = 0; i < size; i++ ) {
		hashtable->table[i] = NULL;
	}

	hashtable->size = size;

	return hashtable;	
}

/* Hash a string for a particular hash table. */
int ht_hash( hashtable_t *hashtable, char *key ) {

	unsigned long int hashval;
	int i = 0;

	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
	}

	return hashval % hashtable->size;
}

/* Create a key-value pair. */
entry_t *ht_newpair( char *key, char *value ) {
	entry_t *newpair;

	if( ( newpair = malloc( sizeof( entry_t ) ) ) == NULL ) {
		return NULL;
	}

	if( ( newpair->key = strdup( key ) ) == NULL ) {
		return NULL;
	}

	if( ( newpair->value = strdup( value ) ) == NULL ) {
		return NULL;
	}

	newpair->next = NULL;

	return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, char *key, char *value ) {
	int bin = 0;
	entry_t *newpair = NULL;
	entry_t *next = NULL;
	entry_t *last = NULL;

	bin = ht_hash( hashtable, key );

	next = hashtable->table[ bin ];

	while( next != NULL && next->key != NULL && strcmp( key, next->key ) > 0 ) {
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {

		free( next->value );
		next->value = strdup( value );

	/* Nope, could't find it.  Time to grow a pair. */
	} else {
		newpair = ht_newpair( key, value );

		/* We're at the start of the linked list in this bin. */
		if( next == hashtable->table[ bin ] ) {
			newpair->next = next;
			hashtable->table[ bin ] = newpair;
	
		/* We're at the end of the linked list in this bin. */
		} else if ( next == NULL ) {
			last->next = newpair;
	
		/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
}

/* Retrieve a key-value pair from a hash table. */
char *ht_get( hashtable_t *hashtable, char *key ) {
	int bin = 0;
	entry_t *pair;

	bin = ht_hash( hashtable, key );

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) > 0 ) {
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
		return NULL;

	} else {
		return pair->value;
	}
	
}



int main( )
{
  
  int length, i = 0;
  int fd;
  int wd;
  char buffer[EVENT_BUF_LEN];
  hashtable_t *hashtable = ht_create( 65536 );

  /*creating the INOTIFY instance*/
  fd = inotify_init();

  /*checking for error*/
  if ( fd < 0 ) {
    perror( "inotify_init" );
  }
  
  char tmp_buffer[2000];
  char tmp_buffer2[20];
  char * tokens;
  char tmp_path[2000];
  char command_str[2500];
  char * base_path = "/home/airavata/inputData";
  /*adding the “/tmp” directory into watch list. Here, the suggestion is to validate the existence of the directory before adding into monitoring list.*/
  wd = inotify_add_watch( fd, "/home/airavata/inputData", IN_CREATE | IN_DELETE );
  snprintf (tmp_buffer, sizeof(tmp_buffer), "key%d",wd);
  ht_set( hashtable, tmp_buffer, "/home/airavata/inputData" );
  	
  /*read to determine the event change happens on “/tmp” directory. Actually this read blocks until the change event occurs*/ 
  for (;;) {		
	  i=0;
	  length = read( fd, buffer, EVENT_BUF_LEN ); 

	  /*checking for error*/
	  if ( length < 0 ) {
	    perror( "read" );
	  }  

	  /*actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.*/
	  while ( i < length ) {     struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];     if ( event->len ) {
	      if ( event->mask & IN_CREATE ) {
		if ( event->mask & IN_ISDIR ) {
  		  int parent_wd = event->wd;
		  snprintf (tmp_buffer, sizeof(tmp_buffer), "key%d",parent_wd);
		  if (ht_get( hashtable, tmp_buffer) != NULL) {
		  	snprintf (tmp_buffer, sizeof(tmp_buffer), "%s/%s", ht_get( hashtable, tmp_buffer), event->name);
		  	printf( "New directory %s created.\n", tmp_buffer);

			int current_wd = inotify_add_watch( fd, tmp_buffer, IN_CREATE | IN_DELETE );
  			snprintf (tmp_buffer2, sizeof(tmp_buffer2), "key%d",current_wd);
  			ht_set( hashtable, tmp_buffer2, tmp_buffer);
		  }	
		}
		else {
		  int parent_wd = event->wd;
		  snprintf (tmp_buffer, sizeof(tmp_buffer), "key%d",parent_wd);	
		  snprintf (tmp_buffer, sizeof(tmp_buffer), "%s/%s", ht_get( hashtable, tmp_buffer), event->name);
		  strcpy(tmp_path, tmp_buffer);
		  tokens = strtok(tmp_path, "/");
		  int i=0;
		  for (i=0; i<3;i++ ) {
		       tokens = strtok(NULL, "/");
		       if (tokens==NULL) break;
		  }
		  printf( "New file with root %s created in path %s.\n", tokens, tmp_buffer);
		  snprintf (command_str, sizeof(command_str), "bash /home/airavata/oodt/inotify/ingester.sh %s %s/%s &", tokens, base_path, tokens);
		  printf ("Command %s \n", command_str);
		  system(command_str);
		}
	      }
	      else if ( event->mask & IN_DELETE ) {
		if ( event->mask & IN_ISDIR ) {
		  printf( "Directory %s deleted.\n", event->name );
		}
		else {
		  printf( "File %s deleted.\n", event->name );
		}
	      }
	    }
	    i += EVENT_SIZE + event->len;
	  }
   }
  /*removing the “/tmp” directory from the watch list.*/
   inotify_rm_watch( fd, wd );

  /*closing the INOTIFY instance*/
   close( fd );

}
