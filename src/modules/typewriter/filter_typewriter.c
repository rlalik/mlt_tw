/*
 * filter_oldfilm.c -- oldfilm filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../qt/kdenlivetitle_wrapper.h"
#include <typewriter.h>

typedef struct {
    TypeWriter tw;          // holds TypeWriter object
    int init;               // 1 if initialized
    int idx_beg;            // index of begin of the pattern
    int idx_end;            // index of the end
    char * data_field;      // data field name
    char * data;            // data field content
    int current_frame;      // currently parsed frame
    int producer_type;      // 1 - kdenlivetitle
    mlt_producer producer;  // hold producer pointer
} twdata;

static int get_producer_data(mlt_properties filter_p, mlt_properties frame_p, twdata * data)
{
    if (data == NULL)
        return 0;

    char data_field[200];
    char * d = NULL;

    mlt_producer producer = NULL;
    mlt_properties producer_properties = NULL;

    /* Obtain the producer for this frame */
    producer_ktitle ktitle = mlt_properties_get_data( frame_p, "producer_kdenlivetitle", NULL );

    if (ktitle != NULL)
    {
        /* Obtain properties of producer */
        producer = &ktitle->parent;
        producer_properties = MLT_PRODUCER_PROPERTIES( producer );

        if (producer == NULL || producer_properties == NULL)
            return 0;

        strcpy(data_field, "xmldata");
        d = mlt_properties_get( producer_properties, data_field );

        int res = -1;
        if (data->data)
            res = strcmp(d, data->data);

        if (res != 0)
        {
            data->init = 0;
        }

        data->producer_type = 1;
        data->producer = producer;
    }

    if (data->init != 0)
        return 1;

    char * str_beg = mlt_properties_get( filter_p, "beg" );
    char * str_end = mlt_properties_get( filter_p, "end" );

    if (d == NULL)
        return 0;

    const char * idx = d;
    int len_beg = strlen(str_beg);
    int len_end = strlen(str_end);
    int i = 0;
    int i_beg = -1;
    int i_end = -1;

    while (*idx != '\0')
    {
        // check first character
        if (*idx != str_beg[0])
        {
            ++i;
            ++idx;
            continue;
        }

        // check full pattern
        if (strncmp(str_beg, idx, len_beg) != 0)
        {
            ++i;
            ++idx;
            continue;
        }

        i_beg = i;

        i += len_beg;
        idx += len_beg;

        while (*idx != '\0')
        {
            // check first character
            if (*idx != str_end[0])
            {
                ++i;
                ++idx;
                continue;
            }

            // check full pattern
            if (strncmp(str_end, idx, len_end) != 0)
            {
                ++i;
                ++idx;
                continue;
            }

            i += len_end;
            idx += len_end;

            i_end = i;

            break;
        }
        break;
    }

    if (i_beg == -1 || i_end == -1)
        return 0;

    int len = i_end - i_beg - len_beg - len_end;    // length of pattern w/o markers
    char * buff = malloc(len);
    memset(buff, 0, len);
    strncpy(buff, d + i_beg + len_beg, len);

    tw_init(&data->tw);
    tw_setRawString(&data->tw, buff);
    /*int res =*/ tw_parse(&data->tw);

    if (data->data_field) free(data->data_field);
    data->data_field = malloc(strlen(data_field));
    strcpy(data->data_field, data_field);

    if (data->data) free(data->data);
    data->data = malloc(strlen(d));
    strcpy(data->data, d);

    data->idx_beg = i_beg;
    data->idx_end = i_end;
    data->current_frame = -1;

    data->init = 1;

    return 1;
}

static int update_producer(mlt_frame frame, mlt_properties frame_p, twdata * data, int restore)
{
    if (data->init == 0)
        return 0;

    mlt_position pos = mlt_frame_original_position(frame);

    mlt_properties producer_properties = NULL;
    if (data->producer_type == 1)
    {
        producer_properties = MLT_PRODUCER_PROPERTIES( data->producer );
        if (restore)
            mlt_properties_set_int( producer_properties, "force_reload", 0 );
        else
            mlt_properties_set_int( producer_properties, "force_reload", 1 );
    }

    if (producer_properties == NULL)
        return 0;

    if (restore == 1)
    {
        mlt_properties_set( producer_properties, data->data_field, data->data );
        return 1;
    }
    int len = data->idx_end - data->idx_beg;
    char * buff1 = malloc(len);
    char * buff2 = malloc(strlen(data->data));
    memset(buff2, 0, strlen(data->data));

    tw_render(&data->tw, pos, buff1, len);
    int len_buff = strlen(buff1);

    strncpy(buff2, data->data, data->idx_beg);
    strncpy(buff2 + data->idx_beg, buff1, len_buff);
    strcpy(buff2 + data->idx_beg + len_buff, data->data + data->idx_end);

    mlt_properties_set( producer_properties, data->data_field, buff2 );

    free(buff2);
    free(buff1);

    data->current_frame = pos;

    return 1;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

    mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

    twdata * data = (twdata*) filter->child;

    int res = get_producer_data(properties, frame_properties, data);
    if ( res == 0)
    {
        return mlt_frame_get_image( frame, image, format, width, height, 1 );
    }

    update_producer(frame, frame_properties, data, 0);

	error = mlt_frame_get_image( frame, image, format, width, height, 1 );

    update_producer(frame, frame_properties, data, 1);

    return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

mlt_filter filter_typewriter_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	twdata* data = (twdata*)calloc( 1, sizeof(twdata) );
    data->init = 0;
    data->data = NULL;
    data->data_field = NULL;

	if ( filter != NULL && data != NULL)
	{
		filter->process = filter_process;
        filter->child = data;
	}
	return filter;
}

