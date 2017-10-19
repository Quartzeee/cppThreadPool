# cppThreadPool

 * cppThreadPool class
 * 
 * Summary:
 * This is a simple RAII pool of threads.  It's important to note that this class
 * will only work with void signatures.  If you want to use any other return type this 
 * class must be enhanced or extended (heavily) since this class is unaware of any
 * returns.  It would be a good idea to make this generic one day to process a broader
 * range of function signatures.
 *
 * Usage:
 * To use this class, declare a ThreadPool and either pass in the number of threads
 * you want to utilize or allow the class to self-determine the number (which will default
 * to the number of hardware threads).  Adding jobs to the class requires you to 
 * bind functions and pass them in as R-values.  This allows the container to have
 * ownership over the call and move any caller resources.
 * To stop the pool, use the kill() method.  This will simply flip our daemon flag
 * and notify all threads to exit.  The caller will have to be careful here since
 * if you kill the threads it doesn't stop the user from adding jobs to the queue.
 * They'll just never be processed it wouldn't be immediately apparent that class
 * has stopped processing (use the bAlive flag to check if need be).
 * 
 */
