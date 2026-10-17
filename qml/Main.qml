import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

ApplicationWindow {
 id: root
 width: 1120; height: 740; minimumWidth: 980; minimumHeight: 620
 visible: true; title: "Lantern"
 color: theme.colors.background
 font.pixelSize: 13
 palette.window: theme.colors.background
 palette.base: theme.colors.lighter_background
 palette.button: theme.colors.lighter_background
 palette.text: theme.colors.foreground
 palette.windowText: theme.colors.foreground
 palette.buttonText: theme.colors.foreground
 palette.highlight: theme.colors.accent
 palette.highlightedText: theme.colors.background
 palette.placeholderText: theme.colors.muted
 property string selectedIp: ""
 property string selectedInfo: ""
 property int sortColumn: 0
 property bool descending: false
 function applyFilter() { if (search && statusFilter && hideDead) hosts.filter(search.text, statusFilter.currentIndex === 0 ? "" : statusFilter.currentText, hideDead.checked) }
 component ActionButton: Button {
  id: button
  padding: 10; leftPadding: 15; rightPadding: 15
  background: Rectangle { radius: 6; color: button.down ? theme.colors.selection : theme.colors.lighter_background; border.color: button.hovered || button.activeFocus ? theme.colors.accent : theme.colors.muted; opacity: button.enabled ? 1 : 0.4 }
 }
 component EntryField: TextField {
  id: field
  padding: 10; selectByMouse: true
  background: Rectangle { radius: 6; color: theme.colors.lighter_background; border.color: field.activeFocus ? theme.colors.accent : theme.colors.muted }
 }
 component Stat: Rectangle {
  property string caption
  property string value
  property color ink: theme.colors.foreground
  Layout.fillWidth: true; implicitHeight: 68; radius: 6
  color: theme.colors.lighter_background
  Column { anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 3
   Label { text: value; color: ink; font.pixelSize: 23; font.bold: true }
   Label { text: caption; color: theme.colors.foreground; opacity: 0.65; font.pixelSize: 11 }
  }
 }
 ColumnLayout {
  anchors.fill: parent; anchors.margins: 20; spacing: 12
  RowLayout {
   Rectangle { width: 42; height: 42; radius: 9; color: theme.colors.lighter_background; border.color: theme.colors.accent
    Canvas { anchors.fill: parent; onPaint: { let c=getContext("2d"); c.reset(); c.strokeStyle=theme.colors.accent; c.lineWidth=2; c.beginPath(); c.arc(21,21,12,0,Math.PI*2); c.moveTo(21,21); c.lineTo(30,12); c.stroke(); c.fillStyle=theme.colors.foreground; c.fillRect(14,23,4,4); c.fillRect(24,27,3,3); }
     Connections { target: theme; function onChanged() { parent.requestPaint() } }
    }
   }
   ColumnLayout { spacing: 2
    Label { text: "Lantern"; font.pixelSize: 24; font.bold: true }
    Label { text: "Network IP scanner"; opacity: 0.65 }
   }
   Item { Layout.fillWidth: true }
   ActionButton { text: "About"; onClicked: about.open() }
  }
  Rectangle {
   Layout.fillWidth: true; implicitHeight: scanControls.implicitHeight+28
   color: "transparent"; radius: 6; border.color: theme.colors.muted
   ColumnLayout { id: scanControls; anchors.fill: parent; anchors.margins: 14; spacing: 10
    RowLayout {
     Label { text: "Scan range"; font.bold: true }
     Item { Layout.fillWidth: true }
     Label { text: "IPv4 · CIDR, range, or single address"; opacity: 0.55; font.pixelSize: 11 }
    }
    RowLayout {
     EntryField { id: range; objectName: "rangeField"; text: scanner.localRange; placeholderText: "192.168.1.0/24"; Layout.fillWidth: true; enabled: !scanner.running; onAccepted: startButton.clicked() }
     ActionButton { text: "Local /24"; enabled: !scanner.running; onClicked: range.text=scanner.localRange }
     ActionButton { id: startButton; objectName: "startButton"; text: scanner.running ? "Scanning…" : "Start scan"; enabled: !scanner.running; onClicked: { root.selectedIp=""; root.selectedInfo=""; scanner.start(range.text, dns.checked, ports.checked, timeout.value, parallel.value) } }
     ActionButton { text: "Stop"; enabled: scanner.running; onClicked: scanner.stop() }
    }
    RowLayout {
     CheckBox { id: dns; text: "Resolve hostnames"; checked: true; enabled: !scanner.running }
     CheckBox { id: ports; text: "Common ports"; checked: true; enabled: !scanner.running; ToolTip.visible: hovered; ToolTip.text: "23 TCP ports: FTP, SSH, Telnet, mail, DNS, web, SMB, print, databases, RDP, VNC" }
     Item { Layout.fillWidth: true }
     Label { text: "Timeout (ms)"; opacity: 0.65 }
     SpinBox { id: timeout; from: 200; to: 5000; stepSize: 100; value: 800; editable: true; enabled: !scanner.running; Layout.preferredWidth: 115 }
     Label { text: "Parallel hosts"; opacity: 0.65 }
     SpinBox { id: parallel; from: 1; to: 64; value: 32; editable: true; enabled: !scanner.running; Layout.preferredWidth: 120 }
    }
   }
  }
  RowLayout { spacing: 10
   Stat { caption: "ADDRESSES"; value: scanner.total.toString() }
   Stat { caption: "LIVE"; value: scanner.alive.toString(); ink: theme.colors.green }
   Stat { caption: "NO REPLY"; value: scanner.silent.toString(); ink: theme.colors.red }
   Stat { caption: "COMPLETED"; value: scanner.completed + " / " + scanner.total }
  }
  RowLayout {
   EntryField { id: search; placeholderText: "Filter by IP, hostname, or port…"; Layout.fillWidth: true; onTextChanged: root.applyFilter() }
   CheckBox { id: hideDead; text: "Hide dead IPs"; onCheckedChanged: root.applyFilter(); ToolTip.visible: hovered; ToolTip.text: "Hide hosts with No reply status" }
   ComboBox { id: statusFilter; model: ["All hosts", "Live", "No reply", "Error", "Cancelled"]; onCurrentIndexChanged: root.applyFilter(); Layout.preferredWidth: 140 }
   ActionButton { text: "Export CSV"; enabled: scanner.total > 0 && !scanner.running; onClicked: exportDialog.open() }
  }
  Rectangle {
   Layout.fillWidth: true; Layout.fillHeight: true; radius: 6; color: "transparent"; border.color: theme.colors.muted; clip: true
   ColumnLayout { anchors.fill: parent; anchors.margins: 1; spacing: 0
    Rectangle { Layout.fillWidth: true; height: 37; color: theme.colors.lighter_background
     RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 12
      Repeater { model: ["IP address", "Status", "Hostname", "Ping", "Open TCP ports"]
       delegate: Label { required property string modelData; required property int index
        text: modelData + (root.sortColumn === index ? (root.descending ? " ↓" : " ↑") : ""); font.bold: true
        Layout.preferredWidth: index===0 ? 140 : index===1 ? 95 : index===3 ? 85 : 0
        Layout.fillWidth: index===2 || index===4
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { root.descending=root.sortColumn===index ? !root.descending : false; root.sortColumn=index; hosts.order(index,root.descending) } }
       }
      }
     }
    }
    ListView {
     id: results; objectName: "results"; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: hosts
     ScrollBar.vertical: ScrollBar {}
     delegate: Rectangle {
      id: row
      required property string ip; required property string status; required property string hostname
      required property string latency; required property string ports; required property string detail; required property int index
      width: results.width; height: 40
      color: root.selectedIp===ip ? theme.colors.selection : (index%2 ? Qt.rgba(0.5,0.5,0.5,0.035) : "transparent")
      RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 12
       Label { text: row.ip; Layout.preferredWidth: 140; font.family: "monospace" }
       Label { text: row.status; Layout.preferredWidth: 95; color: row.status==="Live" ? theme.colors.green : row.status==="No reply" || row.status==="Error" ? theme.colors.red : theme.colors.foreground; opacity: row.status==="Queued" ? 0.4 : 1 }
       Label { text: row.hostname || "—"; Layout.fillWidth: true; Layout.preferredWidth: 0; elide: Text.ElideRight }
       Label { text: row.latency || "—"; Layout.preferredWidth: 85 }
       Label { text: row.ports || "—"; Layout.fillWidth: true; Layout.preferredWidth: 0; elide: Text.ElideRight }
      }
      MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton
       onClicked: mouse => { root.selectedIp=row.ip; root.selectedInfo=[row.ip,row.status,row.hostname,row.latency,row.ports,row.detail].filter(s=>s.length).join(" · "); if(mouse.button===Qt.RightButton) rowMenu.popup() }
       onDoubleClicked: scanner.copy(row.ip)
      }
      Menu { id: rowMenu; MenuItem { text: "Copy IP address"; onTriggered: scanner.copy(row.ip) } MenuItem { text: "Copy host details"; onTriggered: scanner.copy(root.selectedInfo) } }
     }
     Label { anchors.centerIn: parent; visible: results.count===0; text: scanner.total===0 ? "Choose a range and start a scan.\nHosts will appear here as they are checked." : "No hosts match your filters."; horizontalAlignment: Text.AlignHCenter; opacity: 0.5; lineHeight: 1.5 }
    }
   }
  }
  Label { text: root.selectedInfo || "Select a host for details · Double-click to copy its IP"; Layout.fillWidth: true; elide: Text.ElideRight; opacity: 0.65; ToolTip.visible: detailsHover.hovered; ToolTip.text: text; HoverHandler { id: detailsHover } }
  ProgressBar { Layout.fillWidth: true; from: 0; to: Math.max(1,scanner.total); value: scanner.completed; implicitHeight: 4 }
  RowLayout { Label { text: scanner.message || "Ready"; Layout.fillWidth: true; elide: Text.ElideRight } Label { text: "No reply can mean ping is blocked"; opacity: 0.5; font.pixelSize: 11 } }
 }
 FileDialog { id: exportDialog; title: "Export scan results"; fileMode: FileDialog.SaveFile; defaultSuffix: "csv"; nameFilters: ["CSV files (*.csv)"]; currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation); onAccepted: scanner.exportCsv(selectedFile) }
 Dialog { id: about; title: "About Lantern"; anchors.centerIn: parent; modal: true; standardButtons: Dialog.Ok; width: 440
  contentItem: Label { text: "Lantern " + Qt.application.version + "\n\nA Qt Quick / C++ network scanner.\nLive Omarchy theme colors, reverse DNS, and common TCP ports.\n\nLive = ICMP reply or an open TCP port.\nNo reply = neither check received a response.\nDNS names depend on your network’s reverse DNS records.\n\nCommon ports: 21, 22, 23, 25, 53, 80, 110, 139, 143, 443, 445, 587, 631, 993, 995, 1433, 3306, 3389, 5432, 5900, 8000, 8080, 8443."; wrapMode: Text.WordWrap }
 }
 Shortcut { sequence: "Ctrl+F"; onActivated: search.forceActiveFocus() }
 Shortcut { sequence: "Escape"; onActivated: { if(scanner.running) scanner.stop() } }
}
