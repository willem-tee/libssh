/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 Aris Adamantiadis <aris@0xbadc0de.be>
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

/* callback.h
 * This file includes the public declarations for the libssh callback mechanism
 */

#ifndef _SSH_CALLBACK_H
#define _SSH_CALLBACK_H

#include <libssh/libssh.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @internal
 * @brief callback to process simple codes
 * @param code value to transmit
 * @param user Userdata to pass in callback
 */
typedef void (*ssh_callback_int) (int code, void *user);

/** @internal
 * @brief callback for data received messages.
 * @param data data retrieved from the socket or stream
 * @param len number of bytes available from this stream
 * @param user user-supplied pointer sent along with all callback messages
 * @returns number of bytes processed by the callee. The remaining bytes will
 * be sent in the next callback message, when more data is available.
 */
typedef int (*ssh_callback_data) (const void *data, size_t len, void *user);

typedef void (*ssh_callback_int_int) (void *user, int code, int errno_code);

typedef int (*ssh_message_callback) (ssh_session, ssh_message message, void *user);
typedef int (*ssh_channel_callback_int) (ssh_channel channel, int code, void *user);
typedef int (*ssh_channel_callback_data) (ssh_channel channel, int code, void *data, size_t len, void *user);
/**
 * @brief SSH authentication callback.
 *
 * @param prompt        Prompt to be displayed.
 * @param buf           Buffer to save the password. You should null-terminate it.
 * @param len           Length of the buffer.
 * @param echo          Enable or disable the echo of what you type.
 * @param verify        Should the password be verified?
 * @param userdata      Userdata to be passed to the callback function. Useful
 *                      for GUI applications.
 *
 * @return              0 on success, < 0 on error.
 */

typedef int (*ssh_auth_callback) (const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata);
/**
 * @brief SSH log callback. All logging messages will go through this callback
 * @param session Current session handler
 * @param priority Priority of the log, the smaller being the more important
 * @param message the actual message
 * @param userdata Userdata to be passed to the callback function.
 */
typedef void (*ssh_log_callback) (ssh_session session, int priority,
    const char *message, void *userdata);

/**
 * @brief SSH Connection status callback.
 * @param session Current session handler
 * @param status Percentage of connection status, going from 0.0 to 1.0
 * once connection is done.
 * @param userdata Userdata to be passed to the callback function.
 */
typedef void (*ssh_status_callback) (ssh_session session, float status,
		void *userdata);

struct ssh_callbacks_struct {
	/** size of this structure. internal, shoud be set with ssh_callbacks_init()*/
	size_t size;
	/** User-provided data. User is free to set anything he wants here */
	void *userdata;
	/** this functions will be called if e.g. a keyphrase is needed. */
	ssh_auth_callback auth_function;
	/** this function will be called each time a loggable event happens. */
	ssh_log_callback log_function;
	/** this function gets called during connection time to indicate the percentage
	 * of connection steps completed.
	 */
  void (*connect_status_function)(void *userdata, float status);
};
typedef struct ssh_callbacks_struct *ssh_callbacks;

/** These are the callbacks exported by the socket structure
 * They are called by the socket module when a socket event appears
 */
struct ssh_socket_callbacks_struct {
  ssh_callback_data data;
  ssh_callback_int controlflow;
  ssh_callback_int_int exception;
  ssh_callback_int_int connected;
  void *user;
};
typedef struct ssh_socket_callbacks_struct *ssh_socket_callbacks;

#define SSH_SOCKET_FLOW_WRITEWILLBLOCK (1<<0)
#define SSH_SOCKET_FLOW_WRITEWONTBLOCK (1<<1)
#define SSH_SOCKET_EXCEPTION_EOF (1<<0)
#define SSH_SOCKET_EXCEPTION_ERROR (1<<1)

#define SSH_SOCKET_CONNECTED_OK (1<<0)
#define SSH_SOCKET_CONNECTED_ERROR (1<<1)
#define SSH_SOCKET_CONNECTED_TIMEOUT (1<<2)

/** Initializes an ssh_callbacks_struct
 * A call to this macro is mandatory when you have set a new
 * ssh_callback_struct structure. Its goal is to maintain the binary
 * compatibility with future versions of libssh as the structure
 * evolves with time.
 */
#define ssh_callbacks_init(p) do {\
	(p)->size=sizeof(*(p)); \
} while(0);

/** @brief Prototype for a packet callback, to be called when a new packet arrives
 * @param session The current session of the packet
 * @param type packet type (see ssh2.h)
 * @param packet buffer containing the packet, excluding size, type and padding fields
 * @param user user argument to the callback
 * and are called each time a packet shows up
 * @returns SSH_PACKET_USED Packet was parsed and used
 * @returns SSH_PACKET_NOT_USED Packet was not used or understood, processing must continue
 */
typedef int (*ssh_packet_callback) (ssh_session session, uint8_t type, ssh_buffer packet, void *user);

/** return values for a ssh_packet_callback */
/** Packet was used and should not be parsed by another callback */
#define SSH_PACKET_USED 1
/** Packet was not used and should be passed to any other callback
 * available */
#define SSH_PACKET_NOT_USED 2


/** @brief This macro declares a packet callback handler
 * @code
 * SSH_PACKET_CALLBACK(mycallback){
 * ...
 * }
 * @endcode
 */
#define SSH_PACKET_CALLBACK(name) \
	int name (ssh_session session, uint8_t type, ssh_buffer packet, void *user)

struct ssh_packet_callbacks_struct {
	/** Index of the first packet type being handled */
	u_int8_t start;
	/** Number of packets being handled by this callback struct */
	u_int8_t n_callbacks;
	/** A pointer to n_callbacks packet callbacks */
	ssh_packet_callback *callbacks;
	void *user;
};
typedef struct ssh_packet_callbacks_struct *ssh_packet_callbacks;

/**
 * @brief Set the callback functions.
 *
 * This functions sets the callback structure to use your own callback
 * functions for auth, logging and status.
 *
 * @code
 * struct ssh_callbacks_struct cb;
 * memset(&cb, 0, sizeof(struct ssh_callbacks_struct));
 * cb.userdata = data;
 * cb.auth_function = my_auth_function;
 *
 * ssh_callbacks_init(&cb);
 * ssh_set_callbacks(session, &cb);
 * @endcode
 *
 * @param  session      The session to set the callback structure.
 *
 * @param  cb           The callback itself.
 *
 * @return 0 on success, < 0 on error.
 */
LIBSSH_API int ssh_set_callbacks(ssh_session session, ssh_callbacks cb);

#ifdef __cplusplus
}
#endif

#endif /*_SSH_CALLBACK_H */
