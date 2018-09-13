package pl.nietamten.DHTxx;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Instant;

import javax.swing.JOptionPane;

import purejavahidapi.HidDevice;
import purejavahidapi.HidDeviceInfo;
import purejavahidapi.InputReportListener;
import purejavahidapi.PureJavaHidApi;

public class DHTdev implements InputReportListener {

	private HidDeviceInfo path;
	private DataHub data;
	private HidDevice dev = null;
	private byte num;

	public DHTdev(HidDeviceInfo path, DataHub data, byte num) {
		this.path = path;
		this.data = data;
		this.num = num;

		try {
			dev = PureJavaHidApi.openDevice(path);
			dev.setInputReportListener(this);
		} catch (IOException e) {
			JOptionPane.showMessageDialog(null, "Fail to open device file (error code in console, it is os specific)",
					"Error", JOptionPane.ERROR_MESSAGE);

			e.printStackTrace();
		}
	}

	public String toString() {
		return path.getDeviceId().toString();
	}

	synchronized public byte GetNum(){
		return num;
	}
	
	public HidDeviceInfo GetInfo() {
		return path;
	}

	// http://stackoverflow.com/questions/21198882/convert-current-datetime-to-long-in-java
	// http://stackoverflow.com/questions/4485128/how-do-i-convert-long-to-byte-and-back-in-java
	// http://stackoverflow.com/questions/80476/how-can-i-concatenate-two-arrays-in-java
	public byte[] longToBytes(long x) {
		ByteBuffer buffer = ByteBuffer.allocate(Long.BYTES);
		buffer.putLong(x);
		return buffer.array();
	}

	public byte[] concat(byte devNum, byte[] time, byte[] probe) {
		// ...
		int timeLen = 8;
		int probeLen = 4;
		byte[] c = new byte[timeLen + probeLen + 1];
		c[0] = devNum;
		System.arraycopy(time, 0, c, 1, timeLen);
		System.arraycopy(probe, 0, c, timeLen + 1, probeLen);
		return c;
	}

	@Override
	synchronized public void onInputReport(HidDevice source, byte reportID, byte[] reportData, int reportLength) {
		long millis = Instant.now().toEpochMilli();
		byte toWrite[] = concat(num, longToBytes(millis), reportData);

		data.write(toWrite);
	}

	synchronized public byte[] GetFeature() {
		byte[] res = new byte[4];
		dev.getFeatureReport(res, 4);
		return res;
	}

	synchronized public void SetFeature(byte[] data) {
		dev.setFeatureReport(data, 4);
	}

}
