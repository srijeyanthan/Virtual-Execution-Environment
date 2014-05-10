public class SriModified {

	public static int index=1;
	public static SriModified ss;
	public int Test(int a) {
		int b = a * 3;
		return b;
	}

	public void dummy() {
	}

	public static void main(String[] args) {
		ss = new SriModified();
		ModifiedMethod();
		index++;
		ModifiedMethod();		
	}
	
	public static void ModifiedMethod()
	{		
		ss = new SriModified();
		int a = 0;
		for (int i = 0; i < 10; ++i) {
			a = ss.Test(6);
			if(index==2)
			{
				System.out.println("The Second round result is ----------------------------- :::"+ a);
			}
			else
			{
				System.out.println("The result is ----------------------------- :::"+ a);
			}
		}
	}

}
