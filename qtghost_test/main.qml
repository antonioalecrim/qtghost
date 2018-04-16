import QtQuick 2.9
import QtQuick.Controls 2.2

ApplicationWindow {
    visible: true
    width: 600
    height: 400
    title: "Ghosts around"

    Slider {
        id: slider
        x: 370
        y: 38
        value: 0.5
    }

    Tumbler {
        id: tumbler
        x: 35
        y: 38
        model: 10
    }

    RangeSlider {
        id: rangeSlider
        x: 370
        y: 104
        first.value: 0.25
        second.value: 0.75
    }

    Switch {
        id: switch1
        x: 415
        y: 166
        text: qsTr("Switch")
    }

    Switch {
        id: switch2
        x: 415
        y: 240
        text: qsTr("Switch")
    }

    Dial {
        id: dial
        x: 144
        y: 46
    }

    Button {
        id: button
        x: 19
        y: 332
        text: qsTr("Button")
    }

    Button {
        id: button1
        x: 420
        y: 332
        text: qsTr("Button")
    }
}
