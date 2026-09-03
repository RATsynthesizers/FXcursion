/**
 * @file      pubsub_cfg.h
 *
 * @details     This is configuration file for pixel driver
 *
 * @version     1.0.0
 *
 * \date        22.08.2025 - 1.0.0 - DVP - First release
 *
 * @copyright   RAT Synthesizers
 */



#ifndef PUBSUB_CFG_H
#define PUBSUB_CFG_H



// Topics quantity
// Valid values: [1 ; 50]
#define TOPICS_MAX_QTY                          (15U)

// Topic name size
// Valid values: [1 ; 50]
#define TOPIC_NAME_MAX_SIZE                     (20U)

// Topic subscribers quantity
// Valid values: [1 ; 5]
#define TOPIC_SUBSCRIBERS_MAX_QTY               (5U)

/*
 * Messages a subscriber's mail queue can hold before PUBSUB_Publish starts
 * refusing them.
 *
 * This was hardcoded to 1 inside PUBSUB_Subscribe. One item is fine for a
 * topic whose reader only ever wants the newest value - telemetry is exactly
 * that - but it is wrong for anything that carries EVENTS, where each message
 * means something on its own. The UI topic is the case in point: two button
 * or encoder events landing between two GUI frames meant the second was
 * silently discarded, because osMailAlloc is called with a zero timeout and
 * neither PUBSUB_Publish's caller nor the return value was checked.
 *
 * Cost is per real subscriber, not per possible one: osMailCreate runs in
 * PUBSUB_Subscribe, so only topics somebody actually reads allocate anything.
 * With the four live subscribers today - UI at 8 bytes, telemetry at 48,
 * recorder config and ACK at 8 - depth 8 costs a little under 600 bytes of
 * FreeRTOS heap in total.
 *
 * If a future topic carries a large struct AND wants a deep queue, give the
 * depth its own field on the topic rather than raising this for everyone.
 */
// Subscriber queue depth, in messages
// Valid values: [1 ; 64]
#define TOPIC_QUEUE_DEPTH                       (8U)



#endif // #ifndef PUBSUB_CFG_H

//****************************************** end of file *******************************************
