/*
 * SpanDSP - a series of DSP components for telephony
 *
 * private/t38_core.h - An implementation of T.38, less the packet exchange part
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2005 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(_SPANDSP_PRIVATE_T38_CORE_H_)
#define _SPANDSP_PRIVATE_T38_CORE_H_

/*!
    Core T.38 state, common to all modes of T.38.
*/
struct t38_core_state_s
{
    /*! \brief Handler routine to transmit IFP packets generated by the T.38 protocol engine */
    t38_tx_packet_handler_t *tx_packet_handler;
    /*! \brief An opaque pointer passed to tx_packet_handler */
    void *tx_packet_user_data;

    /*! \brief Handler routine to process received indicator packets */
    t38_rx_indicator_handler_t *rx_indicator_handler;
    /*! \brief Handler routine to process received data packets */
    t38_rx_data_handler_t *rx_data_handler;
    /*! \brief Handler routine to process the missing packet condition */
    t38_rx_missing_handler_t *rx_missing_handler;
    /*! \brief An opaque pointer passed to any of the above receive handling routines */
    void *rx_user_data;

    /*! NOTE - Bandwidth reduction shall only be done on suitable Phase C data, i.e., MH, MR
        and - in the case of transcoding to JBIG - MMR. MMR and JBIG require reliable data
        transport such as that provided by TCP. When transcoding is selected, it shall be
        applied to every suitable page in a call. */

    /*! \brief Method 1: Local generation of TCF (required for use with TCP).
               Method 2: Transfer of TCF is required for use with UDP (UDPTL or RTP).
               Method 2 is not recommended for use with TCP. */
    int data_rate_management_method;
    
    /*! \brief The emitting gateway may indicate a preference for either UDP/UDPTL, or
               UDP/RTP, or TCP for transport of T.38 IFP Packets. The receiving device
               selects the transport protocol. */
    int data_transport_protocol;

    /*! \brief Indicates the capability to remove and insert fill bits in Phase C, non-ECM
        data to reduce bandwidth in the packet network. */
    int fill_bit_removal;

    /*! \brief Indicates the ability to convert to/from MMR from/to the line format to
        improve the compression of the data, and reduce the bandwidth, in the
        packet network. */
    int mmr_transcoding;

    /*! \brief Indicates the ability to convert to/from JBIG to reduce bandwidth. */
    int jbig_transcoding;

    /*! \brief For UDP (UDPTL or RTP) modes, this option indicates the maximum
               number of octets that can be stored on the remote device before an
               overflow condition occurs. It is the responsibility of the transmitting
               application to limit the transfer rate to prevent an overflow. The
               negotiated data rate should be used to determine the rate at which
               data is being removed from the buffer. */
    int max_buffer_size;

    /*! \brief This option indicates the maximum size of a UDPTL packet or the
               maximum size of the payload within an RTP packet that can be accepted 
               by the remote device. */
    int max_datagram_size;

    /*! \brief This is the version number of ITU-T Rec. T.38. New versions shall be
               compatible with previous versions. */
    int t38_version;

    /*! \brief Allow time for TEP playout */
    int allow_for_tep;

    /*! \brief The fastest data rate supported by the T.38 channel. */
    int fastest_image_data_rate;

    /*! \brief The number of times each packet type will be sent (low byte). The 
               depth of redundancy (2nd byte). Higher numbers may increase reliability
               for UDP transmission. Zero is valid for the indicator packet category,
               to suppress all indicator packets (typicaly for TCP transmission). */
    int category_control[5];

    /*! \brief TRUE if IFP packet sequence numbers are relevant. For some transports, like TPKT
               over TCP they are not relevent. */
    int check_sequence_numbers;

    /*! \brief The sequence number for the next packet to be transmitted */
    int tx_seq_no;
    /*! \brief The sequence number expected in the next received packet */
    int rx_expected_seq_no;

    /*! \brief The current receive indicator - i.e. the last indicator received */
    int current_rx_indicator;
    /*! \brief The current receive data type - i.e. the last data type received */
    int current_rx_data_type;
    /*! \brief The current receive field type - i.e. the last field_type received */
    int current_rx_field_type;
    /*! \brief The current transmit indicator - i.e. the last indicator transmitted */
    int current_tx_indicator;
    /*! \brief The bit rate for V.34 operation */
    int v34_rate;

    /*! A count of missing receive packets. This count might not be accurate if the
        received packet numbers jump wildly. */
    int missing_packets;

    /*! \brief Error and flow logging control */
    logging_state_t logging;
};

#endif
/*- End of file ------------------------------------------------------------*/
