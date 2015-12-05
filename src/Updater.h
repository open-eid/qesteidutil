#include <QDialog>

class UpdaterPrivate;
class Updater: public QDialog
{
	Q_OBJECT
public:
	explicit Updater(const QString &reader, QWidget *parent = 0);
	~Updater();
	int exec();

Q_SIGNALS:
	void log(const QString &msg);
	void send(const QVariantHash &data);
	void handle(const QByteArray &cmd);

private:
	void process(const QByteArray &data);

	UpdaterPrivate *d;
};
