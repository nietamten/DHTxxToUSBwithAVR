package pl.nietamten.DHTxx;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.EventQueue;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JSlider;
import javax.swing.JTextField;
import javax.swing.SwingWorker;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

import org.knowm.xchart.XChartPanel;
import org.knowm.xchart.XYChart;
import org.knowm.xchart.XYChartBuilder;

import purejavahidapi.HidDeviceInfo;
import purejavahidapi.PureJavaHidApi;

public class DataWindow extends JFrame implements ActionListener, ListSelectionListener {

	private final byte FLAG_DHT22 = 0b00000001;
	private final byte FLAG_NO_AUTODETECT = 0b00000010;
	// private final byte FLAG_DEBUG_OUTPUT = 0b00000100;
	private final byte FLAG_CHANGE_TO_FAST_TIMER_STROBE = 0b00001000;
	private final byte FLAG_CHANGE_TO_SLOW_TIMER_STROBE = 0b00010000;
	private final byte FLAG_LED_OUT_STROBE = 0b00100000;

	private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
	/**
	 * 
	 */
	private static final long serialVersionUID = -6442339283148169538L;
	private JPanel contentPane;
	private JTextField txtRefreshRate;
	private JTextField textField_1;
	private JTextField textField_2;
	private JRadioButton rdbtnDth11;
	private JRadioButton rdbtnDth22;
	private ButtonGroup rdbtngrpDHTtype;
	private JCheckBox chckbxAutoChengeType;
	private JSlider slider_1;
	private JSlider slider_2;
	private int lastCount;

	// private JCheckBox chckbxSzybkiLicznik;
	private JCheckBox chckbxJasneLed;
	private XChartPanel<XYChart> chartPanel;
	private XYChart chart;

	private DefaultListModel<DHTdev> listData = new DefaultListModel<DHTdev>();
	private JList<DHTdev> list = null;

	private DataHub dataHub = null;

	/**
	 * Launch the application.
	 */
	public static void Show(String fileName) {
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					DataWindow frame = new DataWindow(fileName);
					frame.setVisible(true);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}

	private static XYChart createChart() {
		XYChart chart = new XYChartBuilder().xAxisTitle("time").yAxisTitle("value").build();
		chart.getStyler().setDatePattern("HH:mm:ss dd");
		chart.getStyler().setDecimalPattern("#0.000");
		chart.getStyler().setMarkerSize(2);
		return chart;
	}

	/**
	 * Create the frame.
	 * 
	 * @throws FileNotFoundException
	 */
	public DataWindow(String fileName) throws FileNotFoundException {

		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setBounds(100, 100, 907, 551);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		setContentPane(contentPane);
		contentPane.setLayout(new BorderLayout(0, 0));

		chart = createChart();
		chartPanel = new XChartPanel<XYChart>(chart);
		contentPane.add(chartPanel, BorderLayout.CENTER);

		JPanel pnlTop = new JPanel();
		contentPane.add(pnlTop, BorderLayout.NORTH);
		pnlTop.setLayout(new BoxLayout(pnlTop, BoxLayout.X_AXIS));

		JPanel panel_1 = new JPanel();
		pnlTop.add(panel_1);
		panel_1.setLayout(new BorderLayout(0, 0));

		JButton btnApplySettings = new JButton("Apply settings ->");
		panel_1.add(btnApplySettings, BorderLayout.SOUTH);
		btnApplySettings.addActionListener(this);
		btnApplySettings.setActionCommand("ApplySettings");

		list = new JList<DHTdev>(listData);
		panel_1.add(list, BorderLayout.CENTER);
		list.addListSelectionListener(this);

		JPanel panel_2 = new JPanel();
		pnlTop.add(panel_2);
		panel_2.setLayout(new BoxLayout(panel_2, BoxLayout.Y_AXIS));

		JPanel panel = new JPanel();
		panel_2.add(panel);
		panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));

		JLabel lblCzasPomidzyPomiarami = new JLabel("Read speed (ms)");
		panel.add(lblCzasPomidzyPomiarami);

		txtRefreshRate = new JTextField();
		panel.add(txtRefreshRate);
		txtRefreshRate.setColumns(10);

		JPanel panel_3 = new JPanel();
		panel_2.add(panel_3);
		panel_3.setLayout(new BoxLayout(panel_3, BoxLayout.Y_AXIS));

		rdbtnDth11 = new JRadioButton("DTH11");
		panel_3.add(rdbtnDth11);

		rdbtnDth22 = new JRadioButton("DHT22");
		panel_3.add(rdbtnDth22);

		rdbtngrpDHTtype = new ButtonGroup();
		rdbtngrpDHTtype.add(rdbtnDth11);
		rdbtngrpDHTtype.add(rdbtnDth22);

		chckbxAutoChengeType = new JCheckBox("Autodetect sensor type");
		panel_2.add(chckbxAutoChengeType);

		// chckbxSzybkiLicznik = new JCheckBox("Szybki licznik");
		// panel_2.add(chckbxSzybkiLicznik);

		chckbxJasneLed = new JCheckBox("Bright LED");
		panel_2.add(chckbxJasneLed);

		JPanel panel_5 = new JPanel();
		pnlTop.add(panel_5);
		panel_5.setLayout(new BoxLayout(panel_5, BoxLayout.Y_AXIS));

		JLabel lblIloDanych = new JLabel("Amount of visible data");
		panel_5.add(lblIloDanych);

		slider_1 = new JSlider();
		panel_5.add(slider_1);

		JLabel lblPomijaneDane = new JLabel("Amount of skipped data");
		panel_5.add(lblPomijaneDane);

		slider_2 = new JSlider();
		panel_5.add(slider_2);

		JButton btnNewButton_1 = new JButton("Set sliders limits");
		btnNewButton_1.setAlignmentX(Component.CENTER_ALIGNMENT);
		panel_5.add(btnNewButton_1);
		btnNewButton_1.addActionListener(this);
		btnNewButton_1.setActionCommand("1");

		JButton btnNewButton_2 = new JButton("Apply sliders values");
		btnNewButton_2.setAlignmentX(Component.CENTER_ALIGNMENT);
		panel_5.add(btnNewButton_2);
		btnNewButton_2.addActionListener(this);
		btnNewButton_2.setActionCommand("2");

		dataHub = new DataHub(fileName, chart,chartPanel);
		scheduler.schedule(new DeviceListUpdater(), 0, TimeUnit.SECONDS);
	}

	@Override
	public void actionPerformed(ActionEvent arg0) {
		if (arg0.getActionCommand().equals("ApplySettings")) {

			byte data[] = new byte[4];

			ByteBuffer buffer = ByteBuffer.allocate(Short.BYTES);
			int rate = Integer.parseInt(txtRefreshRate.getText());
			if (rate > 65535) {
				rate=65535;
				txtRefreshRate.setText("65535");
			}
			buffer.putChar((char) rate);
			data[1] = buffer.get(1);
			data[2] = buffer.get(0);

			byte flags = 0;
			if (rdbtnDth22.isSelected()) {
				flags |= FLAG_DHT22;
			}
			if (!chckbxAutoChengeType.isSelected()) {
				flags |= FLAG_NO_AUTODETECT;
			}
			// if(chckbxSzybkiLicznik.isSelected()){
			// flags |= FLAG_CHANGE_TO_FAST_TIMER_STROBE;
			// }
			// else {
			// flags |= FLAG_CHANGE_TO_SLOW_TIMER_STROBE;
			// }
			// flags|FLAG_DEBUG_OUTPUT;
			if (chckbxJasneLed.isSelected()) {
				flags |= FLAG_LED_OUT_STROBE;
			}

			data[3] = flags;

			list.getSelectedValue().SetFeature(data);
		} else if (arg0.getActionCommand().equals("1")) {
			slider_1.setMaximum(dataHub.GetCount());
			slider_2.setMaximum(dataHub.GetCount());
			lastCount = dataHub.GetCount();
		} else if (arg0.getActionCommand().equals("2")) {
			int skip = slider_2.getValue();
			int maxCount = slider_1.getValue();
			boolean autoscroll = (skip == 0) || (skip + maxCount > lastCount);
			if (maxCount == slider_1.getMaximum())
				maxCount = 0;
			dataHub.Reset(maxCount, skip, autoscroll);
		}

	}

	class DeviceListUpdater extends SwingWorker<List<HidDeviceInfo>, Object> {

		@Override
		public List<HidDeviceInfo> doInBackground() {
			List<HidDeviceInfo> devList = null;
			try {
				devList = PureJavaHidApi.enumerateDevices();

			} catch (Exception e) {
				e.printStackTrace();
			}
			return devList;
		}

		@Override
		protected void done() {
			List<HidDeviceInfo> devList = null;
			try {
				devList = get();
			} catch (InterruptedException e) {
				e.printStackTrace();
			} catch (ExecutionException e) {
				e.printStackTrace();
			}
			if (devList != null)
				try {

					// dopisywanie nowych
					for (HidDeviceInfo info : devList) {
						if (info.getVendorId() == 0x16c0 && info.getProductId() == 0x05df) {
							boolean present = false;
							for (int i = 0; i < list.getModel().getSize(); i++) {
								if (list.getModel().getElementAt(i).GetInfo().getPath().equals(info.getPath())) {
									present = true;
									break;
								}
							}
							if (!present) {
								byte num = 0;
								boolean found = true;
								while (found) {
									found = false;
									for (int i = 0; i < list.getModel().getSize(); i++) {
										if (num == list.getModel().getElementAt(i).GetNum()) {
											found = true;
											num++;
											break;
										}
									}
								}

								listData.addElement(new DHTdev(info, dataHub, num));
							}
						}
					}
					// usuwanie brakujacych
					LinkedList<Integer> toDel = new LinkedList<Integer>();
					for (int i = 0; i < list.getModel().getSize(); i++) {
						boolean present = false;
						for (HidDeviceInfo info : devList) {
							if (info.getVendorId() == 0x16c0 && info.getProductId() == 0x05df) {
								if (list.getModel().getElementAt(i).GetInfo().getPath().equals(info.getPath())) {
									present = true;
									break;
								}
							}
						}
						if (!present) {
							toDel.add(i);
						}
					}
					ListIterator<Integer> li = toDel.listIterator(toDel.size());
					while (li.hasPrevious()) {
						listData.remove(li.previous());
					}

				} catch (Exception ignore) {
					ignore.printStackTrace();
				}
			scheduler.schedule(new DeviceListUpdater(), 3, TimeUnit.SECONDS);
		}
	}

	public int msecsOfFeatureReport(byte[] bytes) {
		ByteBuffer buffer = ByteBuffer.allocate(Short.BYTES);
		buffer.put(bytes, 2, 1);
		buffer.put(bytes, 1, 1);
		buffer.flip();// need flip
		short tmp = buffer.getShort();
		int msecs = tmp >= 0 ? tmp : 0x10000 + tmp;
		return msecs;
	}

	@Override
	public void valueChanged(ListSelectionEvent arg0) {
		DeviceSettingsSetter dss = new DeviceSettingsSetter();
		dss.execute();

	}

	class DeviceSettingsSetter extends SwingWorker<byte[], Object> {

		@Override
		protected byte[] doInBackground() throws Exception {
			return list.getSelectedValue().GetFeature();
		}

		@Override
		protected void done() {
			byte[] fRep = null;
			try {
				fRep = get();
			} catch (InterruptedException e) {
				e.printStackTrace();
			} catch (ExecutionException e) {
				e.printStackTrace();
			}
			if (fRep != null) {
				txtRefreshRate.setText(Integer.toString(msecsOfFeatureReport(fRep)));
				byte flags = fRep[3];

				if ((flags & FLAG_DHT22) != 0) {
					rdbtnDth22.setSelected(true);
					rdbtnDth11.setSelected(false);
				} else {
					rdbtnDth22.setSelected(false);
					rdbtnDth11.setSelected(true);
				}
				chckbxAutoChengeType.setSelected(((flags & FLAG_NO_AUTODETECT) == 0));
				// debug.setSelected(((flags|FLAG_DEBUG_OUTPUT)!=0));
				// chckbxSzybkiLicznik.setSelected(((flags&FLAG_CHANGE_TO_SLOW_TIMER_STROBE)!=0));
				chckbxJasneLed.setSelected(((flags & FLAG_LED_OUT_STROBE) != 0));
			}
		}
	}

}
