/* -*- c -*- *****************************************************************
** $Id: RandomTransform.c,v 1.11 2006/07/10 13:04:21 kmorel Exp $
**
** Copyright (C) 2003 Sandia Corporation
** Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
** license for use of this work by or on behalf of the U.S. Government.
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that this Notice and any statement
** of authorship are reproduced on all copies.
**
** Test that has each processor draw a randomly placed quadrilateral.
** Makes sure that all compositions are equivalent.
*****************************************************************************/

#include <GL/ice-t.h>
#include <context.h>
#include "test_codes.h"
#include "test-util.h"
#include "glwin.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

static int tile_dim;

static void draw(void)
{
    printf("In draw\n");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_QUADS);
      glVertex3f(-1.0, -1.0, 0.0);
      glVertex3f(1.0, -1.0, 0.0);
      glVertex3f(1.0, 1.0, 0.0);
      glVertex3f(-1.0, 1.0, 0.0);
    glEnd();
    printf("Leaving draw\n");
}

#define DIFF(x, y)      ((x) < (y) ? (y) - (x) : (x) - (y))

static int compare_color_buffers(int local_width, int local_height,
                                 GLubyte *refcbuf, int rank)
{
    int ref_off_x, ref_off_y;
    int bad_pixel_count;
    int x, y;
    char filename[FILENAME_MAX];
    GLubyte *cb;

    printf("Checking returned image.\n");
    cb = icetGetColorBuffer();
    ref_off_x = (rank%tile_dim) * local_width;
    ref_off_y = (rank/tile_dim) * local_height;
    bad_pixel_count = 0;
#define CBR(x, y) (cb[(y)*local_width*4 + (x)*4 + 0])
#define CBG(x, y) (cb[(y)*local_width*4 + (x)*4 + 1])
#define CBB(x, y) (cb[(y)*local_width*4 + (x)*4 + 2])
#define CBA(x, y) (cb[(y)*local_width*4 + (x)*4 + 3])
#define REFCBUFR(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 0])
#define REFCBUFG(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 1])
#define REFCBUFB(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 2])
#define REFCBUFA(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 3])
/* #define CB_EQUALS_REF(x, y)                  \ */
/*     (   (CBR((x), (y)) == REFCBUFR((x) + ref_off_x, (y) + ref_off_y) )\ */
/*      && (CBG((x), (y)) == REFCBUFG((x) + ref_off_x, (y) + ref_off_y) )\ */
/*      && (CBB((x), (y)) == REFCBUFB((x) + ref_off_x, (y) + ref_off_y) )\ */
/*      && (   CBA((x), (y)) == REFCBUFA((x) + ref_off_x, (y) + ref_off_y)\ */
/*       || CBA((x), (y)) == 0 ) ) */
#define CB_EQUALS_REF(x, y)                     \
    (   (DIFF(CBR((x), (y)), REFCBUFR((x) + ref_off_x, (y) + ref_off_y)) < 5) \
     && (DIFF(CBG((x), (y)), REFCBUFG((x) + ref_off_x, (y) + ref_off_y)) < 5) \
     && (DIFF(CBB((x), (y)), REFCBUFB((x) + ref_off_x, (y) + ref_off_y)) < 5) \
     && (DIFF(CBA((x), (y)), REFCBUFA((x) + ref_off_x, (y) + ref_off_y)) < 5) )

    for (y = 0; y < local_height; y++) {
        for (x = 0; x < local_width; x++) {
            if (!CB_EQUALS_REF(x, y)) {
              /* Uh, oh.  Pixels don't match.  This could be a genuine
               * error or it could be a floating point offset when
               * projecting edge boundries to pixels.  If the latter is the
               * case, there will be very few errors.  Count the errors,
               * and make sure there are not too many.  */
                bad_pixel_count++;
            }
        }
    }

  /* Check to make sure there were not too many errors. */
    if (   (bad_pixel_count > 0.001*local_width*local_height)
        && (bad_pixel_count > local_width)
        && (bad_pixel_count > local_height) )
    {
      /* Too many errors.  Call it bad. */
        printf("Too many bad pixels!!!!!!\n");
      /* Write current images. */
        sprintf(filename, "ref%03d.ppm", rank);
        write_ppm(filename, refcbuf, SCREEN_WIDTH, SCREEN_HEIGHT);
        sprintf(filename, "bad%03d.ppm", rank);
        write_ppm(filename, cb, local_width, local_height);
      /* Write difference image. */
        for (y = 0; y < local_height; y++) {
            for (x = 0; x < local_width; x++) {
                int off_x = x + ref_off_x;
                int off_y = y + ref_off_y;
                if (CBR(x, y) < REFCBUFR(off_x, off_y)){
                    CBR(x,y) = REFCBUFR(off_x,off_y) - CBR(x,y);
                } else {
                    CBR(x,y) = CBR(x,y) - REFCBUFR(off_x,off_y);
                }
                if (CBG(x, y) < REFCBUFG(off_x, off_y)){
                    CBG(x,y) = REFCBUFG(off_x,off_y) - CBG(x,y);
                } else {
                    CBG(x,y) = CBG(x,y) - REFCBUFG(off_x,off_y);
                }
                if (CBB(x, y) < REFCBUFB(off_x, off_y)){
                    CBB(x,y) = REFCBUFB(off_x,off_y) - CBB(x,y);
                } else {
                    CBB(x,y) = CBB(x,y) - REFCBUFB(off_x,off_y);
                }
            }
        }
        sprintf(filename, "diff%03d.ppm", rank);
        write_ppm(filename, cb, local_width, local_height);
        return 0;
    }
#undef CBR
#undef CBG
#undef CBB
#undef CBA
#undef REFCBUFR
#undef REFCBUFG
#undef REFCBUFB
#undef REFCBUFA
#undef CB_EQUALS_REF

    return 1;
        
}

static int compare_depth_buffers(int local_width, int local_height,
                                 GLuint *refdbuf, int rank)
{
    int ref_off_x, ref_off_y;
    int bad_pixel_count;
    int x, y;
    char filename[FILENAME_MAX];
    GLuint *db;

    printf("Checking returned image.\n");
    db = icetGetDepthBuffer();
    ref_off_x = (rank%tile_dim) * local_width;
    ref_off_y = (rank/tile_dim) * local_height;
    bad_pixel_count = 0;

    for (y = 0; y < local_height; y++) {
        for (x = 0; x < local_width; x++) {
            if (DIFF(db[y*local_width + x],
                     refdbuf[(y+ref_off_y)*SCREEN_WIDTH
                            +x + ref_off_x]) > 0x0000FFFF) {
              /* Uh, oh.  Pixels don't match.  This could be a genuine
               * error or it could be a floating point offset when
               * projecting edge boundries to pixels.  If the latter is the
               * case, there will be very few errors.  Count the errors,
               * and make sure there are not too many.  */
                bad_pixel_count++;
            }
        }
    }

  /* Check to make sure there were not too many errors. */
    if (   (bad_pixel_count > 0.001*local_width*local_height)
        && (bad_pixel_count > local_width)
        && (bad_pixel_count > local_height) )
    {
      /* Too many errors.  Call it bad. */
        printf("Too many bad pixels!!!!!!\n");

      /* Write encoded image. */
        for (y = 0; y < local_height; y++) {
            for (x = 0; x < local_width; x++) {
                GLuint ref = refdbuf[(y+ref_off_y)*SCREEN_WIDTH
                                    +x + ref_off_x];
                GLuint rendered = db[y*local_width + x];
                GLubyte *encoded = (GLubyte *)&db[y*local_width+x];
                long error = ref - rendered;
                if (error < 0) error = -error;
                encoded[0] = (error & 0xFF000000) >> 24;
                encoded[1] = (error & 0x00FF0000) >> 16;
                encoded[2] = (error & 0x0000FF00) >> 8;
            }
        }
        sprintf(filename, "depth_error%03d.ppm", rank);
        write_ppm(filename, (GLubyte*)db,
                  local_width, local_height);

        return 0;
    }
    return 1;
}

static void check_results(int result)
{
    int rank, num_proc;
    int fail = (result != TEST_PASSED);

    icetGetIntegerv(ICET_RANK, &rank);
    icetGetIntegerv(ICET_NUM_PROCESSES, &num_proc);

    if (rank+1 < num_proc) {
        int in_fail;
        ICET_COMM_RECV(&in_fail, 1, ICET_INT, rank+1, 1111);
        fail |= in_fail;
    }
    if (rank-1 >= 0) {
        ICET_COMM_SEND(&fail, 1, ICET_INT, rank-1, 1111);
        ICET_COMM_RECV(&fail, 1, ICET_INT, rank-1, 2222);
    }
    if (rank+1 < num_proc) {
        ICET_COMM_SEND(&fail, 1, ICET_INT, rank+1, 2222);
    }

    if (fail) {
        finalize_test(TEST_FAILED);
        exit(TEST_FAILED);
    }
}

int RandomTransform()
{
    int i, x, y;
    GLubyte *cb;
    GLubyte *refcbuf = NULL;
    GLubyte *refcbuf2 = NULL;
    GLuint *db;
    GLuint *refdbuf = NULL;
    int result = TEST_PASSED;
    GLfloat mat[16];
    int rank, num_proc;
    GLint *image_order;
    GLint rep_group_size;
    GLint *rep_group;
    GLfloat color[3];
    GLfloat background_color[3];
    unsigned int seed;

    icetGetIntegerv(ICET_RANK, &rank);
    icetGetIntegerv(ICET_NUM_PROCESSES, &num_proc);

    seed = time(NULL) + 10*num_proc*rank;
    printf("Process %d seeding random numbers with %u\n", rank, seed);
    srand(seed);

  /* Decide on an image order and data replication group size. */
    image_order = malloc(num_proc * sizeof(GLint));
    if (rank == 0) {
        for (i = 0; i < num_proc; i++) image_order[i] = i;
        printf("Image order:\n");
        for (i = 0; i < num_proc; i++) {
            int swap_idx = rand()%(num_proc-i) + i;
            int swap = image_order[swap_idx];
            image_order[swap_idx] = image_order[i];
            image_order[i] = swap;
            printf("%4d", image_order[i]);
        }
        printf("\n");
        if (rand()%2) {
          /* No data replication. */
            rep_group_size = 1;
        } else {
            rep_group_size = rand()%num_proc + 1;
        }
        printf("Data replication group sizes: %d\n", rep_group_size);
        for (i = 1; i < num_proc; i++) {
            ICET_COMM_SEND(image_order, num_proc, ICET_INT, i, 30);
            ICET_COMM_SEND(&rep_group_size, 1, ICET_INT, i, 31);
        }
    } else {
        ICET_COMM_RECV(image_order, num_proc, ICET_INT, 0, 30);
        ICET_COMM_RECV(&rep_group_size, 1, ICET_INT, 0, 31);
    }
    icetCompositeOrder(image_order);

  /* Agree on background color. */
    if (rank == 0) {
        background_color[0] = (float)rand()/(float)RAND_MAX;
        background_color[1] = (float)rand()/(float)RAND_MAX;
        background_color[2] = (float)rand()/(float)RAND_MAX;
        for (i = 1; i < num_proc; i++) {
            ICET_COMM_SEND(background_color, 3, ICET_FLOAT, i, 32);
        }
    } else {
        ICET_COMM_RECV(background_color, 3, ICET_FLOAT, 0, 32);
    }

  /* Set up ICE-T. */
    icetDrawFunc(draw);
    icetBoundingBoxf(-1.0, 1.0, -1.0, 1.0, -0.125, 0.125);
    icetEnable(ICET_CORRECT_COLORED_BACKGROUND);

  /* Set up OpenGL. */
    glClearColor(background_color[0], background_color[1],
                 background_color[2], 1.0);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

  /* Get random transformation matrix. */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(2.0f*rand()/RAND_MAX - 1.0f,
                 2.0f*rand()/RAND_MAX - 1.0f,
                 2.0f*rand()/RAND_MAX - 1.0f);
    glRotatef(360.0f*rand()/RAND_MAX, 0.0f, 0.0f,
              (float)rand()/RAND_MAX);
    glScalef((float)(1.0/sqrt(num_proc) - 1.0)*(float)rand()/RAND_MAX + 1.0f,
             (float)(1.0/sqrt(num_proc) - 1.0)*(float)rand()/RAND_MAX + 1.0f,
             1.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);

  /* Set up data replication groups and ensure that they all share the same
   * transformation. */
  /* Pick a color based on my index into the object ordering. */
    for (i = 0; image_order[i] != rank; i++);
    i /= rep_group_size;
    printf("My data replication group: %d\n", i);
    icetDataReplicationGroupColor(i);
    if ((i&0x07) == 0) {
        color[0] = 0.5;  color[1] = 0.5;  color[2] = 0.5;
    } else {
        color[0] = 1.0f*((i&0x01) == 0x01);
        color[1] = 1.0f*((i&0x02) == 0x02);
        color[2] = 1.0f*((i&0x04) == 0x04);
    }
  /* Get the true group size. */
    icetGetIntegerv(ICET_DATA_REPLICATION_GROUP_SIZE, &rep_group_size);
    rep_group = icetUnsafeStateGet(ICET_DATA_REPLICATION_GROUP);
    if (rep_group[0] == rank) {
        for (i = 1; i < rep_group_size; i++) {
            ICET_COMM_SEND(mat, 16, ICET_FLOAT, rep_group[i], 40);
        }
    } else {
        ICET_COMM_RECV(mat, 16, ICET_FLOAT, rep_group[0], 40);
    }

    printf("Transformation:\n");
    printf("    %f %f %f %f\n", mat[0], mat[4], mat[8], mat[12]);
    printf("    %f %f %f %f\n", mat[1], mat[5], mat[9], mat[13]);
    printf("    %f %f %f %f\n", mat[2], mat[6], mat[10], mat[14]);
    printf("    %f %f %f %f\n", mat[3], mat[7], mat[11], mat[15]);

  /* Let everyone get a base image for comparison. */
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    icetStrategy(ICET_STRATEGY_SERIAL);
    icetResetTiles();
    for (i = 0; i < num_proc; i++) {
        icetAddTile(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, i);
    }

    printf("\nGetting base images for z compare.\n");
    icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT,
                           ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT);
    glColor4f(color[0], color[1], color[2], 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(mat);
    icetDrawFrame();
    swap_buffers();

    cb = icetGetColorBuffer();
    refcbuf = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    memcpy(refcbuf, cb, SCREEN_WIDTH * SCREEN_HEIGHT * 4);

    db = icetGetDepthBuffer();
    refdbuf = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(GLuint));
    memcpy(refdbuf, db, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(GLuint));

    printf("Getting base image for color blend.\n");
    icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT, ICET_COLOR_BUFFER_BIT);
    icetEnable(ICET_ORDERED_COMPOSITE);
    glColor4f(0.5f*color[0], 0.5f*color[1], 0.5f*color[2], 0.5);
    icetDrawFrame();
    swap_buffers();

    cb = icetGetColorBuffer();
    refcbuf2 = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    memcpy(refcbuf2, cb, SCREEN_WIDTH * SCREEN_HEIGHT * 4);

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    for (i = 0; i < STRATEGY_LIST_SIZE; i++) {
        GLboolean test_ordering;

        icetStrategy(strategy_list[i]);
        printf("\n\nUsing %s strategy.\n", icetGetStrategyName());

        icetGetBooleanv(ICET_STRATEGY_SUPPORTS_ORDERING, &test_ordering);

        for (tile_dim = 1; tile_dim*tile_dim <= num_proc; tile_dim++) {
            int local_width = SCREEN_WIDTH/tile_dim;
            int local_height = SCREEN_HEIGHT/tile_dim;
            int viewport_width = SCREEN_WIDTH, viewport_height = SCREEN_HEIGHT;
            int viewport_offset_x = 0, viewport_offset_y = 0;

            printf("\nRunning on a %d x %d display.\n", tile_dim, tile_dim);
            icetResetTiles();
            for (y = 0; y < tile_dim; y++) {
                for (x = 0; x < tile_dim; x++) {
                    icetAddTile(x*local_width, y*local_height,
                                local_width, local_height, y*tile_dim + x);
                }
            }

            if (tile_dim > 1) {
                viewport_width
                    = rand()%(SCREEN_WIDTH-local_width) + local_width;
                viewport_height
                    = rand()%(SCREEN_HEIGHT-local_height) + local_height;
            }
            if (viewport_width < SCREEN_WIDTH) {
                viewport_offset_x = rand()%(SCREEN_WIDTH-viewport_width);
            }
            if (viewport_width < SCREEN_HEIGHT) {
                viewport_offset_y = rand()%(SCREEN_HEIGHT-viewport_height);
            }
            
/*          glViewport(viewport_offset_x, viewport_offset_y, */
/*                     viewport_width, viewport_height); */
/*          glViewport(0, 0, local_width, local_height); */

            printf("\nDoing color buffer.\n");
            icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT|ICET_DEPTH_BUFFER_BIT,
                                   ICET_COLOR_BUFFER_BIT);
            icetDisable(ICET_ORDERED_COMPOSITE);

            printf("Rendering frame.\n");
            glColor4f(color[0], color[1], color[2], 1.0);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(-1, (float)(2*local_width*tile_dim)/SCREEN_WIDTH-1,
                    -1, (float)(2*local_height*tile_dim)/SCREEN_HEIGHT-1,
                    -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf(mat);
            icetDrawFrame();
            swap_buffers();

            if (rank < tile_dim*tile_dim) {
                if (!compare_color_buffers(local_width, local_height,
                                           refcbuf, rank)) {
                    result = TEST_FAILED;
                }
            } else {
                printf("Not a display node.  Not testing image.\n");
            }
            check_results(result);

            printf("\nDoing depth buffer.\n");
            icetInputOutputBuffers(ICET_DEPTH_BUFFER_BIT,ICET_DEPTH_BUFFER_BIT);

            printf("Rendering frame.\n");
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(-1, (float)(2*local_width*tile_dim)/SCREEN_WIDTH-1,
                    -1, (float)(2*local_height*tile_dim)/SCREEN_HEIGHT-1,
                    -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf(mat);
            icetDrawFrame();
            swap_buffers();

            if (rank < tile_dim*tile_dim) {
                if (!compare_depth_buffers(local_width, local_height,
                                           refdbuf, rank)) {
                    result = TEST_FAILED;
                }
            } else {
                printf("Not a display node.  Not testing image.\n");
            }
            check_results(result);

            if (test_ordering) {
                printf("\nDoing blended color buffer.\n");
                icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT,
                                       ICET_COLOR_BUFFER_BIT);
                icetEnable(ICET_ORDERED_COMPOSITE);

                printf("Rendering frame.\n");
                glColor4f(0.5f*color[0], 0.5f*color[1], 0.5f*color[2], 0.5);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(-1, (float)(2*local_width*tile_dim)/SCREEN_WIDTH-1,
                        -1, (float)(2*local_height*tile_dim)/SCREEN_HEIGHT-1,
                        -1, 1);
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(mat);
                icetDrawFrame();
                swap_buffers();

                if (rank < tile_dim*tile_dim) {
                    if (!compare_color_buffers(local_width, local_height,
                                               refcbuf2, rank)) {
                        result = TEST_FAILED;
                    }
                } else {
                    printf("Not a display node.  Not testing image.\n");
                }
                check_results(result);
                icetDisable(ICET_ORDERED_COMPOSITE);
            } else {
                printf("\nStrategy does not support ordering, skipping.\n");
            }
        }
    }

    printf("Cleaning up.\n");
    free(image_order);
    free(refcbuf);
    free(refdbuf);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT,
                           ICET_COLOR_BUFFER_BIT);

    finalize_test(result);
    return result;
}
