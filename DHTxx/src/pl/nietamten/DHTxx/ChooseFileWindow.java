package pl.nietamten.DHTxx;

import java.awt.BorderLayout;
import java.awt.EventQueue;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.EmptyBorder;
import javax.swing.filechooser.FileNameExtensionFilter;

public class ChooseFileWindow extends JFrame implements ActionListener {

	static ChooseFileWindow frame = null;

	/**
	 * 
	 */
	private static final long serialVersionUID = -5888953915833634992L;
	private JPanel contentPane;
	static private JTextField txtEwr;
	private JButton btnWska;

	/**
	 * Launch the application.
	 */
	public static void Show() {
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					frame = new ChooseFileWindow();
					frame.setVisible(true);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}

	/**
	 * Create the frame.
	 * 
	 * @throws IOException
	 */
	public ChooseFileWindow() {
		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		setBounds(100, 100, 360, 111);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPane.setLayout(new BorderLayout(0, 0));
		setContentPane(contentPane);

		JButton btnNewButton = new JButton("OK");
		contentPane.add(btnNewButton, BorderLayout.SOUTH);
		btnNewButton.addActionListener(this);
		btnNewButton.setActionCommand("confirm");

		JLabel lblNazwaPliku = new JLabel("Data file");
		contentPane.add(lblNazwaPliku, BorderLayout.NORTH);

		txtEwr = new JTextField();
		txtEwr.setText("data.dat");
		contentPane.add(txtEwr, BorderLayout.CENTER);
		txtEwr.setColumns(10);

		btnWska = new JButton("Select");
		contentPane.add(btnWska, BorderLayout.EAST);
		btnWska.addActionListener(this);
		btnWska.setActionCommand("selectFile");

		try {
			read();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		if ("selectFile".equals(e.getActionCommand())) {
			JFileChooser chooser = new JFileChooser();
			FileNameExtensionFilter filter = new FileNameExtensionFilter("dat", "dat", "d");
			chooser.setFileFilter(filter);
			int returnVal = chooser.showOpenDialog(frame);
			if (returnVal == JFileChooser.APPROVE_OPTION) {
				txtEwr.setText(chooser.getSelectedFile().getPath());
			}
		} else if ("confirm".equals(e.getActionCommand())) {
			frame.setVisible(false);
			try {
				write();
			} catch (IOException e1) {
				e1.printStackTrace();
			}
			DataWindow.Show(txtEwr.getText());
		}

	}

	public static void read() throws IOException {
		FileInputStream in = null;
		String res = new String();
		try {
			in = new FileInputStream("lastFileName.txt");

			int c;
			while ((c = in.read()) != -1) {
				res += (char)c;
			}
		} finally {
			if (in != null) {
				in.close();
			}
		}
		txtEwr.setText(res);
	}

	public static void write() throws IOException {

		FileWriter fw = null;
		File out = null;

		try {

			out = new File("lastFileName.txt");

			fw = new FileWriter(out);
			fw.write(txtEwr.getText());

		} finally {
			if (fw != null) {
				fw.close();
			}
		}
	}

}
