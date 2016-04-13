// Apple Timer
// Copyright � Glenn Fiedler
// http://www.pixeltoaster.com

class AppleTimer : public TimerInterface
{
public:
	
	AppleTimer()
	{
		reset();
	}
	
	void reset()
	{
		Microseconds((UnsignedWide*)&_timeCounter);
		_deltaCounter = _timeCounter;
		_time = 0;
	}
	
	double time()
	{
		UInt64 counter;
		Microseconds((UnsignedWide*)&counter);
		UInt64 delta = counter - _timeCounter;
		_timeCounter = counter;
		_time += delta / 1000000.0;
		return _time;
	}
	
	double delta()
	{
		UInt64 counter;
		Microseconds((UnsignedWide*)&counter);
		UInt64 delta = counter - _deltaCounter;
		_deltaCounter = counter;
		return delta / 1000000.0;
	}
	
	double resolution()
	{
		return 1.0 / 1000000.0;		// microseconds
	}
	
	void wait(double seconds)
	{
		UInt64 counter;
		Microseconds((UnsignedWide*)&counter);
		UInt64 finish = counter + UInt64(seconds*1000000);
		while (counter<finish)
			Microseconds((UnsignedWide*)&counter);
	}
	
private:
	
	double _time;               ///< current time in seconds
	UInt64 _timeCounter;        ///< time counter in microseconds
	UInt64 _deltaCounter;		///< delta counter in microseconds
};
