
/*
Copyright 1987-2012 Robert B. K. Dewar and Mark Emmer.
Copyright 2012-2017 David Shields
*/

#include "port.h"

#include <fcntl.h>

/*
/   swcoup( oupptr )
/
/   swcoup() switches between two output files:  the standard output file
/   provided by the shell and the optional output file provided by the
/   -o option on the command line.
/
/   This switching is necessary so that we blend into the Un*x environment
/   like other programs.  To this end output is routed to the appropriate
/   output file:
/
/    program listing, compilation statisitics, execution statistics,
/    and dump of variables at termination go to the -o file, if
/    specified.
/
/    standard output produced by the executing program goes to the
/    standard output file provided by the shell.
/
/   This routing insures that the ONLY standard output produced by the
/   "spitbol" command is that generated by the spitbol program being
/   executed!  Thus, spitbol can be used as a filter.
/
/
/   There are three calls to swcoup() as described by this sequence of events
/
/       spitbol initialization
/    0->swcoup() called prior to compilation
/       compilation (with output routed to -o file)
/    1->swcoup() called after compilation and prior to execution
/          (with output routed to shell's standard output)
/    2->swcoup() called after execution
/       post mortem activities (with output routed to -o file)
/
/
/   A filename consisting of a single hyphen '-' represents file
/   descriptor 1 provided by the shell.
/
/   Parameters:
/    oupptr    pointer to -o option argument from command line
/   Returns:
/    0 if switch successful / -1 if switch failed
*/

int
swcoup(char *oupptr)
{
    int retval = 0;

    /*
       /   No switch necessary if no -o option or previous errors encountered.
     */
    char namebuf[256];

    if(errflag)
        return 0;

    /* if no output file specified, but listing requested, use input name */
    if(oupptr == 0) {
        if(((spitflag & NOLIST) == 0) && (**inpptr)) {
            appendext(*inpptr, LISTEXT, namebuf, 1);
            oupptr = namebuf;
        } else
            goto swcexit;
    }

    /*
       /   If -o file name is '-' then continue to write file
       /   descriptor 1 provided by the shell.
     */
    if(*oupptr == '-' && *(oupptr + 1) == '\0')
        goto swcexit;

    /*
       /   Do output file switch based on current state:
     */
    switch(oupState++) {

        /*
               /    State 0 (1st call to swcoup):  standard output -> -o file
             */
    case 0:
        origoup = dup(1);                          /* save std output */
        close(1);                                  /* close std output */
        if(appendext(oupptr, LISTEXT, namebuf, 0)) /* Append .lst if needed */
            oupptr = namebuf;
        if((spit_open(oupptr, O_WRONLY | O_CREAT | O_TRUNC,
                      IO_PRIVATE | IO_DENY_READWRITE /* 0666 */,
                      IO_REPLACE_IF_EXISTS | IO_CREATE_IF_NOT_EXIST)) <
           0) { /* create -o file */
            wrterr("-o file open error.");
            ++errflag;
            dup(origoup);
            close(origoup);
            retval = -1;
        }
        break;

        /*
               /    State 1 (2nd call to swcoup):  standard output -> shell output file
             */
    case 1:
        close(1);       /* close -o file */
        dup(origoup);   /* restore std output */
        close(origoup); /* close its duplicate */
        break;

        /*
               /    State 2 (3rd call to swcoup):  standard output -> -o file
             */
    case 2:
        close(1);                                  /* close std output */
        if(appendext(oupptr, LISTEXT, namebuf, 0)) /* Append .lst if needed */
            oupptr = namebuf;
        if((spit_open(oupptr, O_WRONLY,
                      IO_PRIVATE | IO_DENY_READWRITE /* 0666 */,
                      IO_OPEN_IF_EXISTS)) < 0) { /* reopen -o file */
            wrterr("error reopening");
        }
        oupeof(); /* seek to EOF on -o file */
        break;

    default:
        wrterr("Internal system error--SWCOUP");
    }

swcexit:
    return retval;
}
