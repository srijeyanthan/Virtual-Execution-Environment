public class Game
{

    
    int factorial(int n) {
        int fact = 1; // this  will be the result
        for (int i = 1; i <= n; i++) {
            fact *= i;
        }
        return fact;
    }

    public void Calc()
    {
          for(int i =1; i <500 ; ++i)
               factorial(i);  

    }
    public void Calc2()
    {
           for(int i =1; i <500 ; ++i)
               factorial(i);


    }
public void Calc3()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);


    }

   
    public static void main(String[] args)
   {

        Game gme = new Game();
    for(int j =0; j <100 ; ++j)
     {
      gme.Calc();
      gme.Calc2();
     }

    System.out.println("****************************************************");
  for(int j =0; j <50000 ; ++j)
     {
      gme.Calc3();
     }
  for(int j =0; j <10 ; ++j)
     {
      gme.Calc();
     }

  for(int j =0; j <10 ; ++j)
     {
      gme.Calc2();
     }

   }

}
