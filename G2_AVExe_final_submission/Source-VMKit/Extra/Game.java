import java.util.Random;

public class Game
{

    
    int factorial(int n) {
        int fact = 1; // this  will be the result
        for (int i = 1; i <= n; i++) {
            fact *= i;
        }
        return fact;
    }

    public void Calc1()
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

 public void Calc4()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);

    }

 public void Calc5()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);

    }

 public void Calc6()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);

    }

 public void Calc7()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);

    }

 public void Calc8()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);

    }

 public void Calc9()
    {
           for(int i =1; i <1000 ; ++i)
               factorial(i);

    }

    public static void main(String[] args)
   {

      Game gg = new Game();
    for(int j =0; j <100 ; ++j)
     {
      gg.Calc1();
      gg.Calc2();
     }

  for(int j =0; j <5000 ; ++j)
     {
      gg.Calc3(); 

       if( j >200)
          {
       gg.Calc1();
       gg.Calc2();

         }
     }
  for(int j =0; j <10 ; ++j)
     {
      gg.Calc1();
   
     }

  for(int j =0; j <10 ; ++j)
     {
      gg.Calc2();
     }
for(int j =0; j <4000 ; ++j)
     {
      gg.Calc4();
      gg.Calc5();
      gg.Calc6();
   
     }

  for(int j =0; j <3000 ; ++j)
     {
      gg.Calc4();
      gg.Calc5();
      gg.Calc6();
      gg.Calc7();
      gg.Calc8();
      gg.Calc9();
     }

   }

}
