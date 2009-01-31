/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

/* $Id: common.h,v 1.4 2003/08/22 22:37:03 kmorel Exp $ */

#ifndef _ICET_STRATEGY_COMMON_H_
#define _ICET_STRATEGY_COMMON_H_

#include <GL/ice-t.h>
#include <image.h>

/* icetRenderTransferFullImages

   Renders all the tiles that are specified in the ICET_CONTAINED_TILES
   state array and sends them to the processors with ranks specified in
   tile_image_dest.  This method is guaranteed not to deadlock.  It only
   uses memory given with the buffer arguments, and will make its best
   efforts to get the graphics and network hardware to run in parallel.

   imageBuffer - A buffer big enough to hold color and/or depth values
	that is ICET_MAX_PIXELS big.  The size can be determined with
	the icetFullImageSize function in image.h.
   inImage, outImage - Two buffers big enough to hold sparse color and
	depth information for an image that is ICET_MAX_PIXELS big.  The
	size can be determined with the icetCompressedBufferSize macro
	in image.h
   num_receiving - number of image this processor is receiving.
   tile_image_dest - if tile t is in ICET_CONTAINED_TILES, then the
	rendered image for tile t is sent to tile_image_dest[t].
*/
void icetRenderTransferFullImages(IceTImage imageBuffer,
				  IceTSparseImage inImage,
				  IceTSparseImage outImage,
				  GLint num_receiving, GLint *tile_image_dest);


/* icetSendRecvLargeMessages

   Similar to icetRenderTransferFullImages except that it works with
   generic data, data generators, and data handlers.  It takes a count of a
   number of messages to be sent and an array of ranks to send to.  Two
   callbacks are required.  One generates the data (so large data may be
   generated JIT to save memory) and the other handles incoming data.  The
   generate callback is run right before the data it returns is sent to a
   particular destination.  This callback will not be called again until
   the memory it returned is no longer in use, so the memory may be reused.
   As large messages come in, the handle callback is called.  As an
   optimization, if a process sends to itself, then that will be the first
   message created.  This gives the callback an opertunity to build its
   local data while waiting for incoming data.  The handle callback returns
   a pointer to a buffer to be used for the next large message receive.  It
   should be common for this message buffer to be reused too.

   numMessagesSending - A count of the number of large messages this
	processor is sending out.
   messageDestinations - An array of size numMessagesSending that contains
   	the ranks of message destinations.
   generateDataFunc - A callback function that generates messages.  The
	function is given the index in messageDestinations and the rank of
	the destination as arguments.  The data of the message and the size
	of the message (in bytes) are returned.  The generateDataFunc will
	not be called again until the returned data is no longer in use.
	Thus the data may be reused.
   handleDataFunc - A callback function that processes messages.  The
	function is given the data buffer and the rank of the process that
	sent it.  The function is expected to return a buffer to use for
	the next message receive.  If the callback is finished with the
	buffer it was given, it is perfectly acceptable to return it again
	for reuse.
   incomingBuffer - A buffer to use for the first incoming message.
   bufferSize - The maximum size of a message.
   
*/
typedef void *(*IceTGenerateData)(GLint id, GLint dest, GLint *size);
typedef void *(*IceTHandleData)(void *, GLint src);
void icetSendRecvLargeMessages(GLint numMessagesSending,
			       GLint *messageDestinations,
			       GLint messagesInOrder,
			       IceTGenerateData generateDataFunc,
			       IceTHandleData handleDataFunc,
			       void *incomingBuffer,
			       GLint bufferSize);


/* icetBswapCompose

   Performs a binary swap composition amongst a subset of processors in the
   current communicator (see context.h).

   compose_group - A mapping of processors from the MPI ranks to the "group"
	ranks.  The composed image ends up in the processor with rank
	compose_group[image_dest].
   group_size - The number of processors in the group.  The compose_group
	array should have group_size entries.
   image_dest - The location of where the final composed image should be
	placed.  It is an index into compose_group, not the actual rank
	of the process.
   imageBuffer - The input image colors and/or depth to be used.  If this
	processor has rank compose_group[compose_group], any output data
	will be put in this buffer.  If the color or depth value is not to
	be computed or this processor is not rank
	compose_group[compose_group], the buffer has undefined partial
	results when the function returns.
   inImage/outImage - two buffers for holding sparse image data.
*/
void icetBswapCompose(GLint *compose_group, GLint group_size, GLint image_dest,
		      IceTImage imageBuffer,
		      IceTSparseImage inImage, IceTSparseImage outImage);

/* icetTreeCompose

   Performs a binary tree composition amongst a subset of processors in the
   current communicator (see context.h).

   compose_group - A mapping of processors from the MPI ranks to the "group"
	ranks.  The composed image ends up in the processor with rank
	compose_group[image_dest].
   group_size - The number of processors in the group.  The compose_group
	array should have group_size entries.
   image_dest - The location of where the final composed image should be
	placed.  It is an index into compose_group, not the actual rank
	of the process.
   imageBuffer - The input image colors and/or depth to be used.  If this
	processor has rank compose_group[image_dest], any output data will
	be put in this buffer.  If the color or depth value is not to be
	computed or this processor is not rank compose_group[image_dest],
	the buffer has undefined partial results when the function returns.
   compressedImageBuffer - a buffer for holding sparse image data in transit.
*/
void icetTreeCompose(GLint *compose_group, GLint group_size, GLint image_dest,
		     IceTImage imageBuffer,
		     IceTSparseImage compressedImageBuffer);

/* icetCascadedCompose

   Performs a composition based on both binary swap and binary tree
   composition algorithms amongst a subset of processors in the current
   communicator (see context.h).

   compose_group - A mapping of processors from the MPI ranks to the "group"
	ranks.  The composed image ends up in the processor with rank
	compose_group[0].
   group_size - The number of processors in the group.  The compose_group
	array should have group_size entries.
   imageBuffer - The input image colors and/or depth to be used.  If this
	processor has rank compose_group[0], any output data will be put
	in this buffer.  If the color or depth value is not to be computed
	or this processor is not rank compose_group[0], the buffer has
	undefined partial results when the function returns.
   inImage/outImage - two buffers for holding sparse image data.
*/
void icetCascadedCompose(GLint *compose_group, GLint group_size,
			 GLint compose_tile,
			 IceTImage imageBuffer,
			 IceTSparseImage inImage, IceTSparseImage outImage);

#define icetAddSentBytes(num_sending)					\
    ((GLint *)icetUnsafeStateGet(ICET_BYTES_SENT))[0] += (num_sending)

#endif /*_ICET_STRATEGY_COMMON_H_*/
