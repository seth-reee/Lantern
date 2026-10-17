#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTcpServer>
#include "scanner.h"
class ScannerTest:public QObject {
 Q_OBJECT
QTemporaryDir fakeDir;
 QByteArray originalPath;
 void fakePing(const QByteArray &body) {
  QFile fake(fakeDir.filePath("ping")); QVERIFY(fake.open(QIODevice::WriteOnly|QIODevice::Truncate)); fake.write("#!/bin/sh\n"+body); fake.close(); QVERIFY(fake.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
 }
private slots:
 void init() { originalPath=qgetenv("PATH"); fakePing("echo '64 bytes: time=0.123 ms'\nexit 0\n"); qputenv("PATH",fakeDir.path().toUtf8()); }
 void cleanup() { qputenv("PATH",originalPath); }
 void ranges() {
  QString error;
  auto addresses=Scanner::parseRange("192.168.7.123/24",error); QCOMPARE(addresses.size(),254); QCOMPARE(addresses.first(),quint32(0xc0a80701));
  QCOMPARE(Scanner::parseRange("10.0.0.0/31",error).size(),2);
  QCOMPARE(Scanner::parseRange("255.255.255.255/32",error).size(),1);
  QCOMPARE(Scanner::parseRange("127.0.0.1-127.0.0.3",error).size(),3);
  for(auto bad:{"10.0.0.0/8","1.2.3.4/33","1.2.3.5-1.2.3.4","localhost","::1","1.2.3.4;echo bad"}) { QVERIFY(Scanner::parseRange(bad,error).isEmpty()); QVERIFY(!error.isEmpty()); }
 }
 void scanLifecycle() {
  Scanner scanner; scanner.start("127.0.0.1",true,false,300,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),5000);
  QCOMPARE(scanner.completed(),1); QCOMPARE(scanner.alive(),1);
  QCOMPARE(scanner.data(scanner.index(0),Scanner::Status).toString(),QString("Live"));
  QTemporaryDir dir; QVERIFY(scanner.exportCsv(QUrl::fromLocalFile(dir.filePath("hosts.csv"))));
  QFile csv(dir.filePath("hosts.csv")); QVERIFY(csv.open(QIODevice::ReadOnly)); QVERIFY(csv.readAll().contains("\"127.0.0.1\",\"Live\""));
  scanner.start("127.0.0.1-127.0.0.10",true,true,200,2); scanner.stop();
  QVERIFY(!scanner.running()); QCOMPARE(scanner.data(scanner.index(9),Scanner::Status).toString(),QString("Cancelled"));
  scanner.start("127.0.0.1",false,false,200,1); QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000); QCOMPARE(scanner.alive(),1);
 }
 void tcpWithoutPing() {
  QTemporaryDir dir; QFile fake(dir.filePath("ping")); QVERIFY(fake.open(QIODevice::WriteOnly)); fake.write("#!/bin/sh\nexit 1\n"); fake.close(); QVERIFY(fake.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
  auto path=qgetenv("PATH"); qputenv("PATH",dir.path().toUtf8());
  QTcpServer server; if(!server.listen(QHostAddress::LocalHost,8080)) QSKIP(qPrintable("Loopback networking unavailable: "+server.errorString()));
  Scanner scanner; scanner.start("127.0.0.1",false,true,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000); qputenv("PATH",path);
  QCOMPARE(scanner.alive(),1); QVERIFY(scanner.data(scanner.index(0),Scanner::Ports).toString().contains("8080"));
 }
 void silentAndErrors() {
  QTemporaryDir dir; QFile fake(dir.filePath("ping")); QVERIFY(fake.open(QIODevice::WriteOnly)); fake.write("#!/bin/sh\nexit 1\n"); fake.close(); fake.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
  auto path=qgetenv("PATH"); qputenv("PATH",dir.path().toUtf8());
  Scanner scanner; scanner.start("127.0.0.1",false,false,200,1); QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000); QCOMPARE(scanner.silent(),1);
  QVERIFY(fake.open(QIODevice::WriteOnly|QIODevice::Truncate)); fake.write("#!/bin/sh\necho 'permission denied' >&2\nexit 2\n"); fake.close();
  scanner.start("127.0.0.1",false,false,200,1); QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000); qputenv("PATH",path);
  QCOMPARE(scanner.silent(),0); QCOMPARE(scanner.data(scanner.index(0),Scanner::Status).toString(),QString("Error"));
 }
 void filtering() {
  Scanner scanner; scanner.start("127.0.0.1",false,false,200,1); QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000);
  HostFilter filter; filter.setSourceModel(&scanner); filter.filter("127.0","Live"); QCOMPARE(filter.rowCount(),1);
  filter.filter("","No reply"); QCOMPARE(filter.rowCount(),0);
 }
};
QTEST_GUILESS_MAIN(ScannerTest)
#include "scanner_test.moc"
