/**
 * @brief Make an update package from a manifest?
 */


// include some common header here. remember, we're packing a bunch of binary
// data together into 1 blob. we need to be able to reverse that on the actual
// hardware when applying this update

// in the nominal case we're going to expose some UART pins (TX/RX) for the user
// to plug in their favorite serial/USB converter. The STM32H7 (APM) UART will
// attempt to run kinda fast. we'll do some benchmarking of just how fast this
// is though

