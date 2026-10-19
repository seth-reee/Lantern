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
 void portalviewHandoff() {
  Scanner scanner;
  QVERIFY(!scanner.addToPortalview("127.0.0.1", "Localhost"));
  QVERIFY(scanner.message().contains("Install Portalview"));
  QFile app(fakeDir.filePath("portalview"));
  QVERIFY(app.open(QIODevice::WriteOnly));
  app.write("#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$0.args\"\n"); app.close();
  QVERIFY(app.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
  QVERIFY(!scanner.addToPortalview("bad host", "Office"));
  QVERIFY(!scanner.addToPortalview("192.168.1.42", "bad\nname"));
  QVERIFY(scanner.addToPortalview("192.168.1.42", "Office PC"));
  QFile args(fakeDir.filePath("portalview.args"));
  QTRY_VERIFY(args.exists() && args.size()>0);
  QVERIFY(args.open(QIODevice::ReadOnly));
  QCOMPARE(args.readAll(), QByteArray("--add-host\n192.168.1.42\n--name\nOffice PC\n"));
 }
 void arpEntries() {
  const QString table="IP address HW type Flags HW address Mask Device\n"
   "192.168.1.2 0x1 0x2 aa:bb:cc:dd:ee:ff * eth0\n"
   "192.168.1.3 0x1 0x0 00:00:00:00:00:00 * eth0\n"
   "192.168.1.4 0x1 0x2 invalid * eth0\n";
  QCOMPARE(Scanner::macFromArp(table,"192.168.1.2"),QString("AA:BB:CC:DD:EE:FF"));
  QVERIFY(Scanner::macFromArp(table,"192.168.1.3").isEmpty());
  QVERIFY(Scanner::macFromArp(table,"192.168.1.4").isEmpty());
  QVERIFY(Scanner::macFromArp(table,"192.168.1.20").isEmpty());
 }
 void elapsedTime() {
  fakePing("while :; do :; done\n");
  Scanner scanner; QCOMPARE(scanner.elapsedMs(),0);
  scanner.start("127.0.0.1",false,false,5000,1);
  QTest::qWait(120); QVERIFY(scanner.elapsedMs()>=100);
  scanner.stop(); const auto stopped=scanner.elapsedMs();
  QTest::qWait(120); QCOMPARE(scanner.elapsedMs(),stopped);
  fakePing("echo 'time=0.123 ms'\nexit 0\n");
  scanner.start("127.0.0.1",false,false,200,1);
  QVERIFY(scanner.elapsedMs()<stopped);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000);
  const auto finished=scanner.elapsedMs();
  QTest::qWait(120); QCOMPARE(scanner.elapsedMs(),finished);
 }
 void ranges() {
  QString error;
  auto addresses=Scanner::parseRange("192.168.7.123/24",error); QCOMPARE(addresses.size(),254); QCOMPARE(addresses.first(),quint32(0xc0a80701));
  QCOMPARE(Scanner::parseRange("10.0.0.0/31",error).size(),2);
  QCOMPARE(Scanner::parseRange("255.255.255.255/32",error).size(),1);
  QCOMPARE(Scanner::parseRange("127.0.0.1-127.0.0.3",error).size(),3);
  for(auto bad:{"10.0.0.0/8","1.2.3.4/33","1.2.3.5-1.2.3.4","localhost","::1","1.2.3.4;echo bad"}) { QVERIFY(Scanner::parseRange(bad,error).isEmpty()); QVERIFY(!error.isEmpty()); }
 }
 void largeRanges() {
  QString error;
  const auto subnet=Scanner::parseRange("192.168.42.99/16",error);
  QVERIFY(error.isEmpty()); QCOMPARE(subnet.size(),65534);
  QCOMPARE(subnet.first(),quint32(0xc0a80001));
  QCOMPARE(subnet.last(),quint32(0xc0a8fffe));
  QVERIFY(subnet.contains(0xc0a80100)); QVERIFY(subnet.contains(0xc0a800ff));
  const auto high=Scanner::parseRange("255.255.0.0/16",error);
  QCOMPARE(high.size(),65534); QCOMPARE(high.last(),quint32(0xfffffffe));
  const auto full=Scanner::parseRange("255.255.0.0-255.255.255.255",error);
  QCOMPARE(full.size(),Scanner::MaxTargets); QCOMPARE(full.last(),quint32(0xffffffff));
  for(auto oversized:{"10.0.0.0/15","0.0.0.0/0","10.0.0.0-10.1.0.0"}) {
   QVERIFY(Scanner::parseRange(oversized,error).isEmpty());
   QVERIFY(error.contains("65,536"));
  }
 }
 void largeScanCancellation() {
  fakePing("while :; do :; done\n");
  Scanner scanner; HostFilter filter; filter.setSourceModel(&scanner);
  scanner.start("127.42.0.0/16",false,false,5000,2);
  QVERIFY(scanner.running()); QCOMPARE(scanner.total(),65534);
  QCOMPARE(filter.rowCount(),65534);
  QSignalSpy updates(&scanner,&QAbstractItemModel::dataChanged);
  scanner.stop(); QVERIFY(!scanner.running()); QCOMPARE(scanner.completed(),0);
  QCOMPARE(updates.size(),1);
  for(int i=0;i<scanner.total();++i) QCOMPARE(scanner.data(scanner.index(i),Scanner::Status).toString(),QString("Cancelled"));
  filter.filter("127.42.255.254","Cancelled"); QCOMPARE(filter.rowCount(),1);
  fakePing("echo 'time=0.123 ms'\nexit 0\n");
  scanner.start("127.0.0.1",false,false,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000);
  QCOMPARE(scanner.alive(),1); QCOMPARE(scanner.total(),1);
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
