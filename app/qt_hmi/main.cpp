#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QTcpSocket>

class HmiWindow : public QWidget {
    Q_OBJECT

public:
    HmiWindow()
        : socket(new QTcpSocket(this)),
        timer(new QTimer(this)),
        reconnectTimer(new QTimer(this)),
        ledOn(false) {
        setWindowTitle("i.MX6ULL Device Monitor");
        resize(420, 360);

        networkLabel = new QLabel("Network: disconnected");
        ledLabel = new QLabel("LED: unknown");
        keyLabel = new QLabel("KEY: unknown");
        eventsLabel = new QLabel("Events: unknown");
        logView = new QTextEdit;
        logView->setReadOnly(true);

        ledButton = new QPushButton("Toggle LED");

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(networkLabel);
        layout->addWidget(ledLabel);
        layout->addWidget(keyLabel);
        layout->addWidget(eventsLabel);
        layout->addWidget(ledButton);
        layout->addWidget(logView);

        reconnectTimer->setInterval(3000);

        connect(
            reconnectTimer,
            &QTimer::timeout,
            this,
            [this]() {
                if (socket->state() ==
                    QAbstractSocket::UnconnectedState) {
                    socket->connectToHost(
                        "192.168.1.101",
                        8000
                    );
                }
            }
        );

        connect(
            socket,
            &QTcpSocket::connected,
            this,
            [this]() {
                networkLabel->setText("Network: connected");

                logView->append("Connected to device");

                reconnectTimer->stop();
                timer->start(1000);
            });

        connect(
            socket,
            &QTcpSocket::disconnected,
            this,
            [this]() {
                networkLabel->setText("Network: disconnected");

                timer->stop();
                reconnectTimer->start();
            });

        connect(
            socket,
            &QTcpSocket::readyRead,
            this,
            &HmiWindow::readMessages);

        connect(
            ledButton,
            &QPushButton::clicked,
            this,
            &HmiWindow::toggleLed);

        connect(
            timer,
            &QTimer::timeout,
            this,
            &HmiWindow::requestStatus);

        connect(
            socket,
            &QAbstractSocket::errorOccurred,
            this,
            [this](QAbstractSocket::SocketError) {
                networkLabel->setText(
                    "Network: error");

                reconnectTimer->start();
            });    

        socket->connectToHost("192.168.1.101", 8000);
    }

private:
    void requestStatus() {
        socket->write("REQ 1 STATUS GET\n");
        socket->write("REQ 2 KEY GET\n");
    }

    void toggleLed() {
        ledOn = !ledOn;

        const QString state =
            ledOn ? "ON" : "OFF";

        socket->write(
            QString("REQ 3 LED SET %1\n")
                .arg(state)
                .toUtf8());
    }

    void readMessages() {
        receiveBuffer += socket->readAll();

        while (true) {
            const int newline =
                receiveBuffer.indexOf('\n');

            if (newline < 0) {
                break;
            }

            const QByteArray line =
                receiveBuffer.left(newline).trimmed();

            receiveBuffer.remove(0, newline + 1);

            const QString message =
                QString::fromUtf8(line);

            logView->append(message);

            if (message.startsWith("EVENT KEY")) {
                if (message.contains("\"pressed\"")) {
                    keyLabel->setText("KEY: pressed");
                } else if (message.contains("\"released\"")) {
                    keyLabel->setText("KEY: released");
                }
                continue;
            }

            if (message.contains("\"led\":\"on\"")) {
                ledOn = true;
                ledLabel->setText("LED: ON");
            } else if (message.contains("\"led\":\"off\"")) {
                ledOn = false;
                ledLabel->setText("LED: OFF");
            }

            if (message.contains("\"events\":")) {
                const int pos =
                    message.indexOf("\"events\":");

                eventsLabel->setText(
                    "Events: " +
                    message.mid(pos + 9));
            }
        }
    }

    QLabel* networkLabel;
    QLabel* ledLabel;
    QLabel* keyLabel;
    QLabel* eventsLabel;
    QPushButton* ledButton;
    QTextEdit* logView;

    QTcpSocket* socket;
    QTimer* timer;
    QTimer* reconnectTimer;
    QByteArray receiveBuffer;
    bool ledOn;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    HmiWindow window;
    window.show();

    return app.exec();
}