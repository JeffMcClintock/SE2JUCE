#ifndef NOTIFY_H__
#define NOTIFY_H__
#include <vector>

class Notifier;

class Notifiable
{
public:

	virtual ~Notifiable();
	virtual void OnNotify(Notifier* sender, int lHint, void* pHint = 0) = 0;
	void AddWatcher( Notifier* p_notifier )
	{
		m_watching.push_back( p_notifier );
	}
	void RemoveWatcher( Notifier* p_notifier );

private:
	std::vector<Notifier*> m_watching;
};

class Notifier
{
public:
	virtual ~Notifier();
	void RegisterObserver(Notifiable* observer);
	void UnRegisterObserver(Notifiable* observer);
	bool IsRegistered(Notifiable* observer);
	bool has_observers()
	{
		return !m_observers.empty();
	}
	void VO_Notify( int lHint, void* pHint = 0);
	void NotifySafe(int p_msg_id);
	void NotifySafe2(int p_msg_id, void* pHint);
	inline void NotifyFast(int p_msg_id, void* pHint = 0)
	{
		for (std::vector<Notifiable*>::iterator it = m_observers.begin(); it != m_observers.end(); ++it)
		{
			(*it)->OnNotify(this, p_msg_id, pHint);
		}
	}
private:
	std::vector<Notifiable*> m_observers;
};

#endif // NOTIFY_H__

