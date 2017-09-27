#pragma once

class QGInput {
protected:
	QGInput() {};
public:
	virtual ~QGInput() {};

	virtual void open() = 0;
};
