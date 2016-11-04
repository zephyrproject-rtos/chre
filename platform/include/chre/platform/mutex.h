// todo: mutex abstraction to support concurrency
//  - init()
//  - lock()
//  - unlock()
//  - future: try_lock(), recursive mutex, etc.

// todo: RAII-style autolock wrapper
//  - constructor: take mutex as arg, call lock
//  - destructor: call unlock

// Note: don't really need to worry about this yet if we run all nanoapps in
// a single thread
