/**
 * @file        pubsub.h
 *
 * @details     Interface of the publisher-subscriber service functionality.
 *
 * \version     1.0.0 - 31.01.2022 - AVV - First release
 */



#ifndef PUBSUB_H
#define PUBSUB_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

///



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/*!
 * \brief Represents the module status bits structure
 */
typedef STD_RESULT (*PUBSUB_NOTIFY_FUNC)(void* const pData,
                                         const U16 nDataLength);


typedef void* SUB_HANDLE;


/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

// Initalize service.
extern STD_RESULT PUBSUB_Init(void);

// Creates topic.
extern STD_RESULT PUBSUB_CreateTopic(char* const pTopicName,
                                     const U16 nTopicDataLength);

/// Subscribe to the topic.
extern SUB_HANDLE PUBSUB_Subscribe(char* const pTopicName,
                                   const PUBSUB_NOTIFY_FUNC pNotifyFunc);

/// Publish to the topic.
extern STD_RESULT PUBSUB_Publish(char* const pTopicName,
                                 void* const pData,
                                 const U16 nDataLength);

/// Receive the data from topic
extern STD_RESULT PUBSUB_Update(const SUB_HANDLE pSubscriberHandle,
                                void* const pDataBuf,
                                const U16 nDataBufferLength,
                                const U32 nTimeout);



#endif
