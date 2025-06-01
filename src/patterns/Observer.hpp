#ifndef HTOP_CLONE_OBSERVER_HPP
#define HTOP_CLONE_OBSERVER_HPP

class IObserver {
public:
	virtual ~IObserver() = default;
	virtual void onUpdate() = 0;
};

#endif