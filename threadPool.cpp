

#include "threadPool.h"

/**
 * Default constructor - this ctor will initialize the pool to contain the number of hardware threads on the machine.
 */
ThreadPool::ThreadPool() : bAlive(true)
{
	this->initThreads(thread::hardware_concurrency());
};

/**
 * Constructor for caller to specify the number of threads it would like the pool to contain.
 * @param iNumThreads
 */
ThreadPool::ThreadPool(const unsigned int& iNumThreads) : bAlive(true)
{
	this->initThreads(iNumThreads);
};

/**
 * Destructor - dtor will stop the daemon and join all working threads.
 */
ThreadPool::~ThreadPool()
{
	this->kill();	// Stop processing
	for(auto &t : this->vThreads) t.join();  // Join all of the threads before destructing
}

/**
 * This is our delegated thread initializer.
 * @param iNumThreads
 */
void ThreadPool::initThreads(const unsigned int& iNumThreads)
{
	for(unsigned int i = 0; i <= iNumThreads; i++) 
	{
		this->vThreads.emplace_back(&ThreadPool::processJobs, this);
	}
}

/**
 * Use this method to pass bound functions or functors to the pool of threads for processing.
 * 
 * For example: threadPool.addJob(bind( voidFuncName, param1, param2, ... ));
 * @param tNewJob
 */
void ThreadPool::addJob(function<void()> &&tNewJob)
{
	lock_guard<mutex> lck(this->_jobsQueueMtx); // Obtain a lock on the jobs queue
	this->qJobs.emplace(move(tNewJob));			// Once that lock is obtained, move our function object into the queue
	this->cvJobs.notify_one();					// Notify a waiting thread that work is available
}

/**
 * This method is used by the threads to wait and listen for more work.  When work arrives,
 * one of the waiting threads is notified and processes a single job.  It then waits for more
 * work or to be signaled to stop by the kill() method.
 */
void ThreadPool::processJobs()
{
	while(this->bAlive)	// Daemon flag
	{	
		function<void()> job;	// job storage
		
		{
			unique_lock<mutex> lck(this->_jobsQueueMtx);	// Obtain a lock on the jobs queue
			this->cvJobs.wait(lck, [&]{ return (!bAlive || !this->qJobs.empty()); });	// Wait for lock, the predicate returns false to continue waiting on spurious wake-ups

			if(!bAlive) return;	// If our daemon has been killed then we are going to exit
				
			job = move(this->qJobs.front());	// grab the job of our locked queue

			this->qJobs.pop(); // Remove the job from the queue
		} // release lock
			
		job(); // Process job (if you have a different signature that accepts a param, it would go here.  i.e. void(int) = job(1))
	}
}

/**
 * This method will stop the daemon and notify all waiting threads to stop processing.
 * This will not join the threads, they will just cease processing.
 */
void ThreadPool::kill()
{
	this->bAlive = false;		// Flip our daemon flag
	this->cvJobs.notify_all();	// Notify all threads to stop waiting and exit
}