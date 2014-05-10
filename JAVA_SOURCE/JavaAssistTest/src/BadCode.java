
public class BadCode {

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		BadCode b=new BadCode();
		b.Calc();
	}
	
	public void Calc()
	{
		double x=0.000000000000000000000000000000000000000000000000000000000000000000001;
		for (int z = 0; z < 200; z++) 
		{
			
			double a=2+x;
			double b=x+a;
			double c=x+b;
			double d=x+c;
			double e=x+d;
			double f=x+e;
			double g=x+f;		
			double h=x+g;
			double i=x+h;
			double j=x+i;
			double k=x+j;
			double l=x+k;
			
			double bb=x+l;
			double cc=x+bb;
			double dd=x+cc;
			double ee=x+dd;
			double ff=x+ee;
			double gg=x+ff;		
			double hh=x+gg;
			double ii=x+hh;
			double jj=x+ii;
			double kk=x+jj;
			double ll=x+kk;
			
			double bbb=x+ll;
			double ccc=x+bbb;
			double ddd=x+ccc;
			double eee=x+ddd;
			double fff=x+eee;
			double ggg=x+fff;		
			double hhh=x+ggg;
			double iii=x+hhh;
			double jjj=x+iii;
			double kkk=x+jjj;
			double lll=x+kkk;
			x+=lll;
			System.out.println("Result = "+lll);
		}
	}

}
