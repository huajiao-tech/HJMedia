package com.HJMediasdk.utils;
	
	public abstract class HJBaseTimerTask implements Runnable
	{
	    /* Lock object for synchronization. It's also used by Timer class. */
	    final Object lock = new Object();

	    /* If timer was cancelled */
	    boolean cancelled;

	    /* Slots used by Timer */
	    long when;

	    long period;

	    boolean fixedRate;

	    /*
	     * The time when task will be executed, or the time when task was launched
	     * if this is task in progress.
	     */
	    private long scheduledTime;

	    /*
	     * Method called from the Timer for synchronized getting of when field.
	     */
	    long getWhen() 
	    {
	        synchronized (lock)
	        {
	            return when;
	        }
	    }

	    /*
	     * Method called from the Timer object when scheduling an event @param time
	     */
	    void setScheduledTime(long time) {
	        synchronized (lock) {
	            scheduledTime = time;
	        }
	    }

	    /*
	     * Is sTimerTask scheduled into any timer?
	     *
	     * @return {@code true} if the timer task is scheduled, {@code false}
	     * otherwise.
	     */
	    boolean isScheduled() {
	        synchronized (lock) {
	            return when > 0 || scheduledTime > 0;
	        }
	    }

	    /**
	     * Creates a new {@code sTimerTask}.
	     */
	    protected HJBaseTimerTask() {
	    }

	    /**
	     * Cancels the {@code sTimerTask} and removes it from the {@code Timer}'s queue. Generally, it
	     * returns {@code false} if the call did not prevent a {@code sTimerTask} from running at
	     * least once. Subsequent calls have no effect.
	     *
	     * @return {@code true} if the call prevented a scheduled execution
	     *         from taking place, {@code false} otherwise.
	     */
	    public boolean cancel()
	    {
	        synchronized (lock) 
	        {
	            boolean willRun = !cancelled && when > 0;
	            cancelled = true;
	            return willRun;
	        }
	    }

	    /**
	     * Returns the scheduled execution time. If the task execution is in
	     * progress it returns the execution time of the ongoing task. Tasks which
	     * have not yet run return an undefined value.
	     *
	     * @return the most recent execution time.
	     */
	    public long scheduledExecutionTime() {
	        synchronized (lock) {
	            return scheduledTime;
	        }
	    }

	    /**
	     * The task to run should be specified in the implementation of the {@code run()}
	     * method.
	     */
	    public abstract void run();
	}