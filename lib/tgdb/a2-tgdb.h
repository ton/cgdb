#ifndef __A2_TGDB_H__
#define __A2_TGDB_H__

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#include "tgdb_types.h"
#include "logger.h"

/*! \file
 * a2-tgdb.h
 * \brief
 * This interface documents the annotate two context.
 */

/** 
 * This struct is a reference to a libannotate-two instance.
 */
struct annotate_two;

/**  
 * This should probably be moved out of a2-tgdb.h
 */
enum annotate_commands {
    /**
	 * Get a list of breakpoints.
	 */
    ANNOTATE_INFO_BREAKPOINTS,

    /**
     * Tell gdb where to send inferior's output
	 */
    ANNOTATE_TTY,

    /**
     * Complete the current console line
	 */
    ANNOTATE_COMPLETE,

    /**
 	 * Show all the sources inferior is made of
	 */
    ANNOTATE_INFO_SOURCES,

    /**
 	 * Shows information on the current source file
	 */
    ANNOTATE_INFO_SOURCE,

    /**
     * Shows information on the current frame
     */
    ANNOTATE_INFO_FRAME,

    /**
     * Get disassembly for the $pc
     */
    ANNOTATE_DISASSEMBLE_PC,

    /**
     * Get disassembly for specified function
     */
    ANNOTATE_DISASSEMBLE_FUNC,

    /**
     * Query if the CLI disassemble command supports mixed source+assembly.
     *
     * Mixed source+assembly mode was added as the /s flag to the CLI
     * disassemble command and as mode 4 to the MI -data-disassemble
     * command.
     *
     * We query the MI command to determine if it supports mode 4, and
     * if it does, we also know that teh cli disassemble command supports
     * /s.
     *
     * The passing case,
     *   (gdb) interpreter-exec mi "-data-disassemble -s 0 -e 0 -- 4"
     *   ^done,asm_insns=[]
     *
     * The failing case,
     *   (gdb) interpreter-exec mi "-data-disassemble -s 0 -e 0 -- 4"
     *   ^error,msg="-data-disassemble: Mode argument must be 0, 1, 2, or 3."
     *
     * If the command comes back as an MI error, we assume /s is not
     * supported.
     *
     * This functionality was added in gdb in commit 6ff0ba5f.
     */
    ANNOTATE_DATA_DISASSEMBLE_MODE_QUERY
};

/******************************************************************************/
/**
 * @name Starting and Stopping Commands.
 * These functions are for starting and stopping the annotate_two context.
 */
/******************************************************************************/

/*@{*/

/** 
 * This invokes a libannotate_two library instance.
 *
 * The client must call this function before any other function in the 
 * tgdb library.
 *
 * \param debugger_path
 * The path to the desired debugger to use. If this is NULL, then just
 * "gdb" is used.
 *
 * \param argc
 * The number of arguments to pass to the debugger
 *
 * \param argv
 * The arguments to pass to the debugger    
 *
 * \param config_dir
 * The current config directory. Files can be stored here.
 *
 * @return
 * NULL on error, A valid descriptor upon success
 */
struct annotate_two *a2_create_context(const char *debugger_path,
        int argc, char **argv, const char *config_dir, struct logger *logger);

/** 
 * This initializes the libannotate_two libarary.
 *  
 * \param a2
 * The annotate two context.
 *
 * \param debugger_stdin
 * Writing to this descriptor, writes to the stdin of the debugger.
 *
 * \param debugger_stdout
 * Reading from this descriptor, reads from the debugger's stdout.
 *
 * \param inferior_stdin
 * Writing to this descriptor, writes to the stdin of the inferior.
 *
 * \param inferior_stdout
 * Reading from this descriptor, reads from the inferior's stdout.
 *
 * @return Retruns
 * 0 on success, otherwise -1 on error.
 */
int a2_initialize(struct annotate_two *a2,
        int *debugger_stdin, int *debugger_stdout,
        int *inferior_stdin, int *inferior_stdout);

/**
 * Shuts down the annotate two context. No more calls can be made on the
 * current context. It will clean up after itself. All descriptors it 
 * opened, it will close.
 *
 * \param ctx
 * The annotate two context.
 *
 * @return
 * 0 on success, otherwise -1 on error.
 */
int a2_shutdown(struct annotate_two *a2);

/*@}*/

/******************************************************************************/
/**
 * @name Status Commands
 * These functions are for querying the annotate_two context.
 */
/******************************************************************************/

/*@{*/

/**
 * This determines if the annotate two context is ready to receive
 * another command.
 *
 * \param ctx
 * The annotate two context.
 *
 * @return
 * 1 if it is ready, 0 if it is not.
 */
int a2_is_client_ready(struct annotate_two *a2);

/** 
 * This lets the annotate_two know that the user ran a command.
 * The client can update itself here if it need to.
 *
 * \param ctx
 * The annotate two context.
 *
 * @return
 * -1 on error, 0 on success
 */
int a2_user_ran_command(struct annotate_two *a2);

/** 
 *  Prepare's the client for the command COM to be run.
 *
 * \param ctx
 * The annotate two context.
 *
 * \param com
 * The command to be run.
 *
 * @return
 * -1 on error, 0 on success
 */
int a2_prepare_for_command(struct annotate_two *a2, struct tgdb_command *com);

/**
 * This is a hack. It should be removed eventually.
 * It tells tgdb-base not to send its internal commands when this is true.
 *
 * \param ctx
 * The annotate two context.
 *
 * @return
 * 1 if it is at a misc prompt, 0 if it is not.
 */
int a2_is_misc_prompt(struct annotate_two *a2);

/*@}*/

/******************************************************************************/
/**
 * @name Input/Output commands
 * These functions are for communicating I/O with an annotate two context.
 */
/******************************************************************************/

/*@{*/

 /** 
  * This receives all of the output from the debugger. It is all routed 
  * through this function. 
  *
  * \param ctx
  * The annotate two context.
  *
  * \param input_data
  * This is the stdout from the debugger. This is the data that parse_io 
  * will parse.
  *
  * \param input_data_size
  * This is the size of input_data.
  *
  * \param debugger_output
  * This is an out variable. It contains data that has been determined to
  * be the output of the debugger that the user should see.
  *
  * \param debugger_output_size
  * This is the size of debugger_output
  *
  * \param inferior_output
  * This is an out variable. It contains data that has been determined to
  * be the output of the inferior that the user should see.
  *
  * \param inferior_output_size
  * This is the size of inferior_output
  *
  * \param list
  * Any commands that the annotate_two context has discovered will
  * be added to the queue Q. This will eventually update the client
  * of the libtgdb library.
  *
  * @return
  * 1 when it has finished a command, 
  * 0 on success but hasn't received enough I/O to finish the command, 
  * otherwise -1 on error.
  */
int a2_parse_io(struct annotate_two *a2,
        const char *input_data, const size_t input_data_size,
        char *debugger_output, size_t * debugger_output_size,
        char *inferior_output, size_t * inferior_output_size,
        struct tgdb_list *list);

/*@}*/

/******************************************************************************/
/**
 * @name Functional commands
 * These functinos are used to ask an annotate_two context to perform a task.
 */
/******************************************************************************/

/*@{*/

int a2_get_current_location(struct annotate_two *a2);

/** 
 * \param ctx
 * The annotate two context.
 *
 * @return 
 * -1 on error. Or pid on Success.
 */
pid_t a2_get_debugger_pid(struct annotate_two *a2);

/*@}*/

/******************************************************************************/
/**
 * @name Inferior tty commands
 * These functinos are used to alter an annotate_two contexts tty state.
 */
/******************************************************************************/

/*@{*/

/** 
 * \param ctx
 * The annotate two context.
 *
 * \param inferior_stdin
 * Writing to this descriptor, writes to the stdin of the inferior.
 *
 * \param inferior_stdout
 * Reading from this descriptor, reads from the inferior's stdout.
 *
 * @return
 * 0 on success, otherwise -1 on error.
 */
int a2_open_new_tty(struct annotate_two *a2, int *inferior_stdin, int *inferior_stdout);

/*@}*/

#endif /* __A2_TGDB_H__ */
