// Test cases from Marek Felšöci

int i = 1;
int x = 2;

int main() {
  {

    int *const apacMemeBloc__a_0 = new int(1);
    int &a = *(apacMemeBloc__a_0);
    ;
    int *const apacMemeBloc__b_1 = new int(i);
    int &b = *(apacMemeBloc__b_1);
    ;
    const int *const apacMemeBloc__c_2 = new const int(1);
    const int &c = *(apacMemeBloc__c_2);
    ;
    const int *const apacMemeBloc__d_3 = new const int(i);
    const int &d = *(apacMemeBloc__d_3);
    ;
    int &e = i;
    const int *const apacMemeBloc__f_4 = new const int(1);
    const int &f = *(apacMemeBloc__f_4);
    ;
    int const &g = i;
    const int *apacMemeBloc__h_5 = new const int[5]{1, 2, 3, 4, 5};
    const int *&h = (apacMemeBloc__h_5);
    ;
    int *apacMemeBloc__j_6 = new int[5]{1, 2, 3, 4, 5};
    int *&j = (apacMemeBloc__j_6);
    ;

    int *const apacMemeBloc__k_7 = new int(1);
    int &k = *(apacMemeBloc__k_7);
    ;
    int *const apacMemeBloc__l_8 = new int(i);
    int &l = *(apacMemeBloc__l_8);
    ;

    const int *const apacMemeBloc__m_9 = new const int(1);
    const int &m = *(apacMemeBloc__m_9);
    ;
    const int *const apacMemeBloc__n_10 = new const int(i);
    const int &n = *(apacMemeBloc__n_10);
    ;

    int &o = i;
    int &p = x;

    const int *const apacMemeBloc__r_11 = new const int(1);
    const int &r = *(apacMemeBloc__r_11);
    ;
    const int &s = i;

    const int *apacMemeBloc__t_12 = new const int[5]{1, 2, 3, 4, 5};
    const int *&t = (apacMemeBloc__t_12);
    ;
    const int *apacMemeBloc__u_13 = new const int[2]{1, 2};
    const int *&u = (apacMemeBloc__u_13);
    ;
    const int *apacMemeBloc__v_14 = new const int[1]{0};
    const int *&v = (apacMemeBloc__v_14);
    ;

    int *apacMemeBloc__w_15 = new int[5]{1, 2, 3, 4, 5};
    int *&w = (apacMemeBloc__w_15);
    ;
    int *apacMemeBloc__y_16 = new int[2]{1, 2};
    int *&y = (apacMemeBloc__y_16);
    ;
    int *apacMemeBloc__z_17 = new int[1]{0};
    int *&z = (apacMemeBloc__z_17);
    ;

    int *aa = &i;
    int *ab = &x;
    int *ac = 0;

    const int *ad = &i;
    const int *ae = &i;
    const int *const af = &x;

    int const *const ag = af;
    int const *const ah = &x;

    int **apacMemeBloc__po_18 = new int *[10];
    int **&po = (apacMemeBloc__po_18);
    ;
    int **apacMemeBloc__pl_19 = new int *[2]{aa, ab};
    int **&pl = (apacMemeBloc__pl_19);
    ;

    int **apacMemeBloc__pg_20 = new int *[2]{aa, ab};
    int **&pg = (apacMemeBloc__pg_20);
    ;
    int *ai = new int(1);

    int(*apacMemeBloc__aj_21)[2][2] =
        new int[2][2][2]{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
    int(*&aj)[2][2] = (apacMemeBloc__aj_21);
    ;
    delete apacMemeBloc__a_0;
    delete apacMemeBloc__b_1;
    delete apacMemeBloc__c_2;
    delete apacMemeBloc__d_3;
    delete apacMemeBloc__f_4;
    delete[] apacMemeBloc__h_5;
    delete[] apacMemeBloc__j_6;
    delete apacMemeBloc__k_7;
    delete apacMemeBloc__l_8;
    delete apacMemeBloc__m_9;
    delete apacMemeBloc__n_10;
    delete apacMemeBloc__r_11;
    delete[] apacMemeBloc__t_12;
    delete[] apacMemeBloc__u_13;
    delete[] apacMemeBloc__v_14;
    delete[] apacMemeBloc__w_15;
    delete[] apacMemeBloc__y_16;
    delete[] apacMemeBloc__z_17;
    delete[] apacMemeBloc__po_18;
    delete[] apacMemeBloc__pl_19;
    delete[] apacMemeBloc__pg_20;
    delete[] apacMemeBloc__aj_21;
  }

  return 0;
}