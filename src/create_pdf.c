/*
 *  create_pdf.c: Routines that create the PDF erasure certificate
 *
 *  Copyright PartialVolume <https://github.com/PartialVolume>.
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdint.h>
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "nwipe.h"
#include "context.h"
#include "create_pdf.h"
#include "PDFGen/pdfgen.h"
#include "version.h"
#include "method.h"
#include "embedded_images/shred_db.jpg.h"
#include "embedded_images/tick_erased.jpg.h"
#include "embedded_images/redcross.h"
#include "logging.h"
#include "options.h"
#include "prng.h"
#include "hpa_dco.h"
#include "miscellaneous.h"

#define text_size_data 10

int create_pdf( nwipe_context_t* ptr )
{
    extern nwipe_prng_t nwipe_twister;
    extern nwipe_prng_t nwipe_isaac;
    extern nwipe_prng_t nwipe_isaac64;

    char pdf_footer[MAX_PDF_FOOTER_TEXT_LENGTH];
    nwipe_context_t* c;
    c = ptr;
    char model_header[50] = ""; /* Model text in the header */
    char serial_header[30] = ""; /* Serial number text in the header */
    char device_size[100] = ""; /* Device size in the form xMB (xxxx bytes) */
    char barcode[100] = ""; /* Contents of the barcode, i.e model:serial */
    char verify[20] = ""; /* Verify option text */
    char blank[10] = ""; /* blanking pass, none, zeros, ones */
    char rounds[50] = ""; /* rounds ASCII numeric */
    char prng_type[50] = ""; /* type of prng, twister, isaac, isaac64 */
    char start_time_text[50] = "";
    char end_time_text[50] = "";
    char bytes_erased[50] = "";
    char HPA_pre_erase[50] = "";
    char HPA_status_text[50] = "";
    char HPA_size_text[50] = "";
    char errors[50] = "";
    char throughput_txt[50] = "";

    float height;
    float page_width;

    struct pdf_info info = { .creator = "https://github.com/PartialVolume/shredos.x86_64",
                             .producer = "https://github.com/martijnvanbrummelen/nwipe",
                             .title = "PDF Disk Erasure Certificate",
                             .author = "Nwipe",
                             .subject = "Disk Erase Certificate",
                             .date = "Today" };

    /* A pointer to the system time struct. */
    struct tm* p;

    /* ------------------ */
    /* Initialise Various */

    // nwipe_log( NWIPE_LOG_NOTICE, "Create the PDF disk erasure certificate" );
    struct pdf_doc* pdf = pdf_create( PDF_A4_WIDTH, PDF_A4_HEIGHT, &info );

    /* Create footer text string and append the version */
    snprintf( pdf_footer, sizeof( pdf_footer ), "Disc Erasure by NWIPE version %s", version_string );

    pdf_set_font( pdf, "Helvetica" );
    struct pdf_object* page_1 = pdf_append_page( pdf );

    /* Obtain page page_width */
    page_width = pdf_page_width( page_1 );

    /* ---------------------------------------------------------- */
    /* Create header and footer, with the exception of the green  */
    /* tick/red icon which is set from the 'status' section below */
    pdf_add_text_wrap( pdf, NULL, pdf_footer, 12, 0, 30, PDF_BLACK, page_width, PDF_ALIGN_CENTER, &height );
    pdf_add_line( pdf, NULL, 50, 50, 550, 50, 3, PDF_BLACK );
    pdf_add_line( pdf, NULL, 50, 650, 550, 650, 3, PDF_BLACK );
    pdf_add_image_data( pdf, NULL, 45, 665, 100, 100, bin2c_shred_db_jpg, 27063 );
    pdf_set_font( pdf, "Helvetica-Bold" );
    snprintf( model_header, sizeof( model_header ), " %s: %s ", "Model", c->device_model );
    pdf_add_text( pdf, NULL, model_header, 14, 195, 755, PDF_BLACK );
    snprintf( serial_header, sizeof( serial_header ), " %s: %s ", "S/N", c->device_serial_no );
    pdf_add_text( pdf, NULL, serial_header, 14, 215, 735, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );
    pdf_add_text( pdf, NULL, "Disk Erasure Report", 24, 190, 690, PDF_BLACK );
    snprintf( barcode, sizeof( barcode ), "%s:%s", c->device_model, c->device_serial_no );
    pdf_add_barcode( pdf, NULL, PDF_BARCODE_128A, 100, 790, 400, 25, barcode, PDF_BLACK );

    /* ------------------------ */
    /* Organisation Information */
    pdf_add_line( pdf, NULL, 50, 550, 550, 550, 1, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Organisation Performing The Disk Erasure", 12, 50, 630, PDF_BLUE );
    pdf_add_text( pdf, NULL, "Business Name:", 12, 60, 610, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Business Address:", 12, 60, 590, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Contact Name:", 12, 60, 570, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Contact Phone:", 12, 300, 570, PDF_GRAY );

    /* -------------------- */
    /* Customer Information */
    pdf_add_line( pdf, NULL, 50, 450, 550, 450, 1, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Customer Details", 12, 50, 530, PDF_BLUE );
    pdf_add_text( pdf, NULL, "Name:", 12, 60, 510, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Address:", 12, 60, 490, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Contact Name:", 12, 60, 470, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Contact Phone:", 12, 300, 470, PDF_GRAY );

    /* ---------------- */
    /* Disk Information */
    pdf_add_line( pdf, NULL, 50, 330, 550, 330, 1, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Disk Information", 12, 50, 430, PDF_BLUE );

    /* Make/model */
    pdf_add_text( pdf, NULL, "Make/Model:", 12, 60, 410, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, c->device_model, text_size_data, 135, 410, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /* Serial no. */
    pdf_add_text( pdf, NULL, "Serial:", 12, 340, 410, PDF_GRAY );
    if( c->device_serial_no[0] == 0 )
    {
        snprintf( c->device_serial_no, sizeof( c->device_serial_no ), "Unknown" );
    }
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, c->device_serial_no, text_size_data, 380, 410, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /* Capacity (Size) of disk */
    pdf_add_text( pdf, NULL, "Size(Apparent): ", 12, 60, 390, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    snprintf( device_size, sizeof( device_size ), "%s, %lli bytes", c->device_size_text, c->device_size );
    pdf_add_text( pdf, NULL, device_size, text_size_data, 145, 390, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    pdf_add_text( pdf, NULL, "Bus:", 12, 340, 390, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, c->device_type_str, text_size_data, 370, 390, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    pdf_add_text( pdf, NULL, "Size(Real):", 12, 60, 370, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    if( !strcmp( c->device_type_str, "NVME" ) || !strcmp( c->device_type_str, "VIRT" )
        || c->HPA_status == HPA_NOT_APPLICABLE )
    {
        snprintf( device_size, sizeof( device_size ), "Not applicable to %s", c->device_type_str );
        pdf_add_text( pdf, NULL, device_size, text_size_data, 125, 370, PDF_DARK_GREEN );
    }
    else
    {
        if( c->HPA_status == HPA_ENABLED )
        {
            snprintf( device_size,
                      sizeof( device_size ),
                      "%s, %lli bytes",
                      c->DCO_reported_real_max_size_text,
                      c->DCO_reported_real_max_size );
            pdf_add_text( pdf, NULL, device_size, text_size_data, 125, 370, PDF_RED );
        }
        else
        {
            if( c->HPA_status == HPA_UNKNOWN )
            {
                snprintf( device_size, sizeof( device_size ), "Unknown" );
                pdf_add_text( pdf, NULL, device_size, text_size_data, 125, 370, PDF_RED );
            }
        }
    }

    pdf_set_font( pdf, "Helvetica" );

    pdf_add_text( pdf, NULL, "Remapped Sectors:", 12, 340, 370, PDF_GRAY );

    /* --------------- */
    /* Erasure Details */
    pdf_add_text( pdf, NULL, "Disk Erasure Details", 12, 50, 310, PDF_BLUE );

    /* start time */
    pdf_add_text( pdf, NULL, "Start time:", 12, 60, 290, PDF_GRAY );
    p = localtime( &c->start_time );
    snprintf( start_time_text,
              sizeof( start_time_text ),
              "%i/%02i/%02i %02i:%02i:%02i",
              1900 + p->tm_year,
              1 + p->tm_mon,
              p->tm_mday,
              p->tm_hour,
              p->tm_min,
              p->tm_sec );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, start_time_text, text_size_data, 120, 290, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /* end time */
    pdf_add_text( pdf, NULL, "End time:", 12, 300, 290, PDF_GRAY );
    p = localtime( &c->end_time );
    snprintf( end_time_text,
              sizeof( end_time_text ),
              "%i/%02i/%02i %02i:%02i:%02i",
              1900 + p->tm_year,
              1 + p->tm_mon,
              p->tm_mday,
              p->tm_hour,
              p->tm_min,
              p->tm_sec );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, end_time_text, text_size_data, 360, 290, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /* Duration */
    pdf_add_text( pdf, NULL, "Duration:", 12, 60, 270, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, c->duration_str, text_size_data, 115, 270, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /*******************
     * Status of erasure
     */
    pdf_add_text( pdf, NULL, "Status:", 12, 300, 270, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );

    if( !strcmp( c->wipe_status_txt, "ERASED" ) )
    {
        pdf_add_text( pdf, NULL, c->wipe_status_txt, 12, 365, 270, PDF_DARK_GREEN );
        pdf_add_ellipse( pdf, NULL, 390, 275, 45, 10, 2, PDF_DARK_GREEN, PDF_TRANSPARENT );

        /* Display the green tick icon in the header */
        pdf_add_image_data( pdf, NULL, 450, 665, 100, 100, bin2c_te_jpg, 54896 );
    }
    else
    {
        if( !strcmp( c->wipe_status_txt, "FAILED" ) )
        {
            // text shifted left slightly in ellipse due to extra character
            pdf_add_text( pdf, NULL, c->wipe_status_txt, 12, 370, 270, PDF_RED );

            // Print the red cross
            pdf_add_image_data( pdf, NULL, 450, 665, 100, 100, bin2c_redcross_jpg, 60331 );
        }
        else
        {
            pdf_add_text( pdf, NULL, c->wipe_status_txt, 12, 360, 270, PDF_RED );

            // Print the red cross
            pdf_add_image_data( pdf, NULL, 450, 665, 100, 100, bin2c_redcross_jpg, 60331 );
        }
        pdf_add_ellipse( pdf, NULL, 390, 275, 45, 10, 2, PDF_RED, PDF_TRANSPARENT );
    }
    pdf_set_font( pdf, "Helvetica" );

    /********
     * Method
     */
    pdf_add_text( pdf, NULL, "Method:", 12, 60, 250, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, nwipe_method_label( nwipe_options.method ), text_size_data, 110, 250, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /***********
     * prng type
     */
    pdf_add_text( pdf, NULL, "PRNG algorithm:", 12, 300, 250, PDF_GRAY );
    if( nwipe_options.method == &nwipe_verify_one || nwipe_options.method == &nwipe_verify_zero
        || nwipe_options.method == &nwipe_zero || nwipe_options.method == &nwipe_one )
    {
        snprintf( prng_type, sizeof( prng_type ), "Not applicable to method" );
    }
    else
    {
        if( nwipe_options.prng == &nwipe_twister )
        {
            snprintf( prng_type, sizeof( prng_type ), "Twister" );
        }
        else
        {
            if( nwipe_options.prng == &nwipe_isaac )
            {
                snprintf( prng_type, sizeof( prng_type ), "Isaac" );
            }
            else
            {
                if( nwipe_options.prng == &nwipe_isaac64 )
                {
                    snprintf( prng_type, sizeof( prng_type ), "Isaac64" );
                }
                else
                {
                    snprintf( prng_type, sizeof( prng_type ), "Unknown" );
                }
            }
        }
    }
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, prng_type, text_size_data, 395, 250, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /******************************************************
     * Final blanking pass if selected, none, zeros or ones
     */
    if( nwipe_options.noblank )
    {
        strcpy( blank, "None" );
    }
    else
    {
        strcpy( blank, "Zeros" );
    }
    pdf_add_text( pdf, NULL, "Final Pass(Zeros/Ones/None):", 12, 60, 230, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, blank, text_size_data, 230, 230, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /* ***********************************************************************
     * Create suitable text based on the numeric value of type of verification
     */
    switch( nwipe_options.verify )
    {
        case NWIPE_VERIFY_NONE:
            strcpy( verify, "Verify None" );
            break;

        case NWIPE_VERIFY_LAST:
            strcpy( verify, "Verify Last" );
            break;

        case NWIPE_VERIFY_ALL:
            strcpy( verify, "Verify All" );
            break;
    }
    pdf_add_text( pdf, NULL, "Verify Pass(Last/All/None):", 12, 300, 230, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, verify, text_size_data, 450, 230, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /* ************
     * bytes erased
     */
    pdf_add_text( pdf, NULL, "*Bytes Erased:", 12, 60, 210, PDF_GRAY );

    /* Use real max sectors taken from DCO if not zero */
    if( c->DCO_reported_real_max_sectors != 0 )
    {
        snprintf( bytes_erased,
                  sizeof( bytes_erased ),
                  "%lli (%.1f%%)",
                  c->bytes_erased,
                  (double) ( (double) c->bytes_erased / (double) ( (double) c->DCO_reported_real_max_sectors * 512 ) )
                      * 100 );
    }
    else
    {
        /* else use the reported size which will be less if there is a enabled HPA */
        snprintf( bytes_erased,
                  sizeof( bytes_erased ),
                  "%lli (%.1f%%)",
                  c->bytes_erased,
                  (double) ( (double) c->bytes_erased / (double) c->device_size ) * 100 );
    }
    pdf_set_font( pdf, "Helvetica-Bold" );
    if( c->bytes_erased == c->device_size )
    {
        pdf_add_text( pdf, NULL, bytes_erased, text_size_data, 145, 210, PDF_DARK_GREEN );
    }
    else
    {
        pdf_add_text( pdf, NULL, bytes_erased, text_size_data, 145, 210, PDF_RED );
    }
    pdf_set_font( pdf, "Helvetica" );

    /************************************************
     * rounds - How many times the method is repeated
     */
    pdf_add_text( pdf, NULL, "Rounds(completed/requested):", 12, 300, 210, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    if( !strcmp( c->wipe_status_txt, "ERASED" ) )
    {
        snprintf( rounds, sizeof( rounds ), "%i/%i", c->round_working, nwipe_options.rounds );
        pdf_add_text( pdf, NULL, rounds, text_size_data, 470, 210, PDF_DARK_GREEN );
    }
    else
    {
        snprintf( rounds, sizeof( rounds ), "%i/%i", c->round_working - 1, nwipe_options.rounds );
        pdf_add_text( pdf, NULL, rounds, text_size_data, 470, 210, PDF_RED );
    }
    pdf_set_font( pdf, "Helvetica" );

    /*******************************
     * HPA, DCO, post erase - LABELS
     */
    pdf_add_text( pdf, NULL, "HPA:", 12, 60, 190, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, HPA_status_text, text_size_data, 155, 190, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );
    pdf_add_text( pdf, NULL, "HPA Size:", 12, 300, 190, PDF_GRAY );

    /*******************
     * Populate HPA size
     */
    pdf_set_font( pdf, "Helvetica-Bold" );
    if( c->HPA_status == HPA_ENABLED )
    {
        snprintf( HPA_size_text, sizeof( HPA_size_text ), "%lli sectors", c->HPA_size );
        pdf_add_text( pdf, NULL, HPA_size_text, text_size_data, 360, 190, PDF_RED );
    }
    else
    {
        if( c->HPA_status == HPA_DISABLED )
        {
            snprintf( HPA_size_text, sizeof( HPA_size_text ), "Zero sectors" );
            pdf_add_text( pdf, NULL, HPA_size_text, text_size_data, 360, 190, PDF_DARK_GREEN );
        }
        else
        {
            if( c->HPA_status == HPA_UNKNOWN )
            {
                snprintf( HPA_size_text, sizeof( HPA_size_text ), "UNKNOWN" );
                pdf_add_text( pdf, NULL, HPA_size_text, text_size_data, 360, 190, PDF_RED );
            }
        }
    }
    pdf_set_font( pdf, "Helvetica" );

    /*********************
     * Populate HPA status (and size if not applicable, NVMe and VIRT)
     */
    if( !strcmp( c->device_type_str, "NVME" ) || !strcmp( c->device_type_str, "VIRT" ) )
    {
        snprintf( HPA_status_text, sizeof( HPA_status_text ), "Not applicable to %s", c->device_type_str );
        pdf_set_font( pdf, "Helvetica-Bold" );
        pdf_add_text( pdf, NULL, HPA_status_text, text_size_data, 95, 190, PDF_DARK_GREEN );
        snprintf( HPA_size_text, sizeof( HPA_size_text ), "Not applicable to %s", c->device_type_str );
        pdf_add_text( pdf, NULL, HPA_size_text, text_size_data, 360, 190, PDF_DARK_GREEN );
        pdf_set_font( pdf, "Helvetica" );
    }
    else
    {
        if( c->HPA_status == HPA_ENABLED )
        {
            snprintf( HPA_status_text, sizeof( HPA_status_text ), "ENABLED" );
            pdf_set_font( pdf, "Helvetica-Bold" );
            pdf_add_text( pdf, NULL, HPA_status_text, text_size_data, 95, 190, PDF_RED );
            pdf_set_font( pdf, "Helvetica" );
        }
        else
        {
            if( c->HPA_status == HPA_DISABLED )
            {
                snprintf( HPA_status_text, sizeof( HPA_status_text ), "DISABLED" );
                pdf_set_font( pdf, "Helvetica-Bold" );
                pdf_add_text( pdf, NULL, HPA_status_text, text_size_data, 95, 190, PDF_DARK_GREEN );
                pdf_set_font( pdf, "Helvetica" );
            }
            else
            {
                if( c->HPA_status == HPA_UNKNOWN )
                {
                    snprintf( HPA_status_text, sizeof( HPA_status_text ), "UNKNOWN" );
                    pdf_set_font( pdf, "Helvetica-Bold" );
                    pdf_add_text( pdf, NULL, HPA_status_text, text_size_data, 95, 190, PDF_RED );
                    pdf_set_font( pdf, "Helvetica" );
                }
            }
        }
    }

    /************
     * Throughput
     */
    pdf_add_text( pdf, NULL, "Throughput:", 12, 300, 170, PDF_GRAY );
    snprintf( throughput_txt, sizeof( throughput_txt ), "%s/sec", c->throughput_txt );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf, NULL, throughput_txt, text_size_data, 370, 170, PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /********
     * Errors
     */
    pdf_add_text( pdf, NULL, "Errors(pass/sync/verify):", 12, 60, 170, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    snprintf( errors, sizeof( errors ), "%llu/%llu/%llu", c->pass_errors, c->fsyncdata_errors, c->verify_errors );
    if( c->pass_errors != 0 || c->fsyncdata_errors != 0 || c->verify_errors != 0 )
    {
        pdf_add_text( pdf, NULL, errors, text_size_data, 195, 170, PDF_RED );
    }
    else
    {
        pdf_add_text( pdf, NULL, errors, text_size_data, 195, 170, PDF_DARK_GREEN );
    }
    pdf_set_font( pdf, "Helvetica" );

    /*************
     * Information
     */
    pdf_add_text( pdf, NULL, "Information:", 12, 60, 150, PDF_GRAY );
    pdf_set_font( pdf, "Helvetica-Bold" );
    pdf_add_text( pdf,
                  NULL,
                  "* bytes erased: The amount of drive that's been erased at least once",
                  text_size_data,
                  60,
                  130,
                  PDF_BLACK );
    pdf_set_font( pdf, "Helvetica" );

    /************************
     * Technician/Operator ID
     */
    pdf_add_line( pdf, NULL, 50, 120, 550, 120, 1, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Technician/Operator ID", 12, 50, 100, PDF_BLUE );
    pdf_add_text( pdf, NULL, "Name/ID:", 12, 60, 80, PDF_GRAY );
    pdf_add_text( pdf, NULL, "Signature:", 12, 300, 100, PDF_BLUE );
    pdf_add_line( pdf, NULL, 360, 65, 550, 66, 1, PDF_GRAY );

    /*****************************
     * Create the reports filename
     *
     * Sanitize the strings that we are going to use to create the report filename
     * by converting any non alphanumeric characters to an underscore or hyphon
     */
    replace_non_alphanumeric( end_time_text, '-' );
    replace_non_alphanumeric( c->device_model, '_' );
    replace_non_alphanumeric( c->device_serial_no, '_' );
    snprintf( c->PDF_filename,
              sizeof( c->PDF_filename ),
              "nwipe_report_%s_Model_%s_Serial_%s.pdf",
              end_time_text,
              c->device_model,
              c->device_serial_no );

    pdf_save( pdf, c->PDF_filename );
    pdf_destroy( pdf );
    return 0;
}
