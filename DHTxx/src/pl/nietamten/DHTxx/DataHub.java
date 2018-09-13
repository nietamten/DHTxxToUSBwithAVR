package pl.nietamten.DHTxx;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Instant;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.CopyOnWriteArrayList;

import javax.swing.SwingWorker;

import org.knowm.xchart.XChartPanel;
import org.knowm.xchart.XYChart;
import org.knowm.xchart.XYSeries;

public class DataHub implements Runnable {

	private FileInputStream in = null;
	private FileOutputStream out = null;
	private int count = 0;

	private String fileName;
	private int maxCount = 0;
	private int skip = 0;
	private boolean reset = false;
	boolean autoscroll = true;

	private XChartPanel<XYChart> chartPanel;
	private final XYChart chart;

	class SensorSeries {
		String sensorID;
		List<Date> xT = new CopyOnWriteArrayList<Date>();
		List<Double> yT = new CopyOnWriteArrayList<Double>();
		List<Date> xH = new CopyOnWriteArrayList<Date>();
		List<Double> yH = new CopyOnWriteArrayList<Double>();
	}

	private Vector<SensorSeries> series;

	public DataHub(String fileName, XYChart chart, XChartPanel<XYChart> chartPanel) throws FileNotFoundException {
		this.chart = chart;
		this.fileName = fileName;
		this.chartPanel = chartPanel;

		series = new Vector<SensorSeries>();
		out = new FileOutputStream(fileName, true);
		in = new FileInputStream(fileName);

		new Thread(this).start();

	}

	synchronized public void write(byte probe[]) {
		try {
			out.write(probe);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public long timeOfProbe(byte[] bytes) {
		ByteBuffer buffer = ByteBuffer.allocate(Long.BYTES);
		buffer.put(bytes, 1, 8);
		buffer.flip();// need flip
		return buffer.getLong();
	}

	public short tempOfProbe(byte[] bytes) {
		ByteBuffer buffer = ByteBuffer.allocate(Short.BYTES);
		buffer.put(bytes, 10, 1);
		buffer.put(bytes, 9, 1);
		buffer.flip();// need flip
		return buffer.getShort();
	}

	public short humOfProbe(byte[] bytes) {
		ByteBuffer buffer = ByteBuffer.allocate(Short.BYTES);
		buffer.put(bytes, 12, 1);
		buffer.put(bytes, 11, 1);
		buffer.flip();// need flip
		return buffer.getShort();
	}

	synchronized public int GetCount() {
		return count;
	}

	synchronized public void Reset(int maxCount, int skip, boolean autoscroll) {
		this.maxCount = maxCount;
		this.skip = skip;
		this.autoscroll = autoscroll;
		count = 0;
		reset = true;
	}

	// http://stackoverflow.com/questions/4142313/java-convert-milliseconds-to-time-format
	// http://stackoverflow.com/questions/7932701/read-byte-as-unsigned-short-java

	class DataReader extends SwingWorker<Object, Object> {

		@Override
		protected byte[] doInBackground() throws Exception {
			return null;
		}

		@Override
		protected void done() {
			chartPanel.updateUI();
		}
	}

	@Override
	public void run() {

		boolean changed;
		while (true) {
			changed = false;
			try {
				if (reset) {
					try {
						in = new FileInputStream(fileName);
					} catch (FileNotFoundException e) {
						e.printStackTrace();
					}
					Map<String, XYSeries> toDel = chart.getSeriesMap();
					
					while(!toDel.isEmpty()) {
						chart.removeSeries(toDel.entrySet().iterator().next().getKey());
					}
					series.clear();
					
					reset = false;
				}

				try {
					while (in.available() >= 13) {
						changed = true;
								
						byte probe[] = new byte[13];
						try {
							in.read(probe, 0, 13);
						} catch (IOException e1) {
							e1.printStackTrace();
						}
						long millis = timeOfProbe(probe);
						Instant instant = Instant.ofEpochMilli(millis);

						short tRaw = tempOfProbe(probe);
						short hRaw = humOfProbe(probe);
						int t = tRaw >= 0 ? tRaw : 0x10000 + tRaw;
						int h = hRaw >= 0 ? hRaw : 0x10000 + hRaw;
						byte devNum = probe[0];

						System.out.println(instant + " d: " + devNum + " t: " + t + " h: " + h);

						while ((series.size() < 2 * (devNum + 1)))
							series.add(new SensorSeries());

						if ((skip == 0 || skip < count) && (autoscroll || count < skip + maxCount)) {
							java.util.Date d = java.util.Date.from(instant);

							SensorSeries ss = series.elementAt(devNum);
							ss.xT.add(d);
							ss.yT.add((double) t / 100);
							ss.xH.add(d);
							ss.yH.add((double) h / 100);

							if (!chart.getSeriesMap()
									.containsKey("Sensor " + Integer.toString(devNum ) + " temp"))
								chart.addSeries("Sensor " + Integer.toString(devNum ) + " temp", ss.xT,
										ss.yT);
							if (!chart.getSeriesMap()
									.containsKey("Sensor " + Integer.toString(devNum  ) + " hum"))
								chart.addSeries("Sensor " + Integer.toString(devNum ) + " hum", ss.xH, ss.yH);

							chart.updateXYSeries("Sensor " + Integer.toString(devNum ) + " temp", ss.xT,
									ss.yT, null);
							chart.updateXYSeries("Sensor " + Integer.toString(devNum ) + " hum", ss.xH, ss.yH,
									null);

						}
						count++;

					}
				} catch (IOException e1) {
					e1.printStackTrace();
				}

			} finally {
				try {
					Thread.sleep(500);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
			if(changed)
				new DataReader().execute();
		}
	}

}
