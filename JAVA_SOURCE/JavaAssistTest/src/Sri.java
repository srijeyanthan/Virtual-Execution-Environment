public class Sri {

	public int Test(int a) {
		int b = a * 3;
		return b;
	}

	public void dummy() {

	}

	public static void main(String[] args) {
		Sri ss = new Sri();
		int a = 0;
		for (int i = 0; i < 10; ++i) {
			a = ss.Test(6);
			System.out
					.println("The result is ----------------------------- :::"
							+ a);
		}
		ss.dummy();
		for (int i = 0; i < 10; ++i) {
			a = ss.Test(6);
			System.out
					.println("The second result is ----------------------------- :::"
							+ a);

		}
	}

}