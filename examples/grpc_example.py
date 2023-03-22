#!/usr/bin/python3
import sys
# append protobuf library location to path
# sys.path.append("../pprzgcs/python")

import grpc
import pprzgcs_pb2 as p
import pprzgcs_pb2_grpc as g
import google.protobuf.json_format as pj
import time

CHANNEL_TIMEOUT = 1


class Demo:
    def __init__(self, addr='localhost', port=50051):
        target = addr + ':' + str(port)
        self.channel = grpc.insecure_channel(target)
        self.stub = g.GCSServiceStub(self.channel)
        crf = grpc.channel_ready_future(self.channel)
        try:
            crf.result(timeout=CHANNEL_TIMEOUT)
        except grpc.FutureTimeoutError as e:
            crf.cancel()
            raise Exception("channel not ready. Is the GCS running?") from e

    def __del__(self):
        self.channel.close()

    def center_ac(self, ac):
        print(f"center on AC {ac}")
        if isinstance(ac, int):
            creq = pj.ParseDict({'aircraft': {'ac_id': ac}}, p.CenterOnRequest())
        elif isinstance(ac, str):
            creq = pj.ParseDict({'aircraft': {'ac_name': ac}}, p.CenterOnRequest())
        else:
            return
        response: p.CenterOnResponse = self.stub.CenterOn(creq)
        status_str = p.CenterOnResponse.CenterSatus.Name(response.status)
        print(f"response: {status_str}, msg:{response.msg}")

    def center_pos(self, lat: float, lon: float):
        print(f"center on pos {lat}, {lon}")
        creq = pj.ParseDict({'coordinate': {'CRS': 'WGS84', 'lat': lat, 'lon': lon}}, p.CenterOnRequest())
        response: p.CenterOnResponse = self.stub.CenterOn(creq)
        status_str = p.CenterOnResponse.CenterSatus.Name(response.status)
        print(f"response: {status_str}, msg:{response.msg}")

    def hide_widget(self, widget_name: str, visible: bool):
        creq = pj.ParseDict({'widget_name': widget_name, 'visible': visible}, p.WidgetProperty())
        self.stub.SetWidgetProperty(creq)

    def set_widget_property(self, widget_name: str, property_name, value):
        creq = pj.ParseDict({'widget_name': widget_name, 'property': {'property_name': property_name}},
                            p.WidgetProperty())
        creq.property.value.Pack(value)
        self.stub.SetWidgetProperty(creq)


if __name__ == "__main__":
    demo = Demo()
    demo.center_pos(43.46, 1.27)
    time.sleep(0.5)
    color = pj.ParseDict({'red': 0, 'green': 255, 'blue': 0, 'alpha': 100}, p.Color())
    demo.set_widget_property("wind_indicator", "background_color", color)

    for i in range(2):
        demo.hide_widget("right_pane", bool(i % 2))
        time.sleep(1)
    demo.center_ac("Bixler")

