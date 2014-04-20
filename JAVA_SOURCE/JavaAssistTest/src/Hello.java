
public class Hello {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		Hello h=new Hello();
		for(int i=0;i<10;i++)
		{
			h.Test();
			h.Happy();
			h.Say();
		}
	}
	
	public void Test()
	{
		System.out.println("This is a test");
	}

	public void Happy()
	{
		System.out.println("Happy happy...!!!");
	}
	
	public void Say()
	{
		System.out.println("Say hi..");
	}
}
