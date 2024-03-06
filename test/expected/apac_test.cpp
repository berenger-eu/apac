// Test cases from Marek Felšöci

int i = 1;
int x = 2;

int main() {
  int *const apacMemeBloc__a_0 = new int(1);
  int &a = *(apacMemeBloc__a_0);

  int *const apacMemeBloc__b_0 = new int(i);
  int &b = *(apacMemeBloc__b_0);

  const int *const apacMemeBloc__c_0 = new const int(1);
  const int &c = *(apacMemeBloc__c_0);

  const int *const apacMemeBloc__d_0 = new const int(i);
  const int &d = *(apacMemeBloc__d_0);

  int &e = i;

  const int *const apacMemeBloc__f_0 = new const int(1);
  const int &f = *(apacMemeBloc__f_0);

  const int &g = i;

  const int *apacMemeBloc__h_0 = new const int[5]{1, 2, 3, 4, 5};
  const int *&h = (apacMemeBloc__h_0);

  int *apacMemeBloc__j_0 = new int[5]{1, 2, 3, 4, 5};
  int *&j = (apacMemeBloc__j_0);

  int *const apacMemeBloc__k_0 = new int(1);
  int &k = *(apacMemeBloc__k_0);
  int *const apacMemeBloc__l_0 = new int(i);
  int &l = *(apacMemeBloc__l_0);

  const int *const apacMemeBloc__m_0 = new const int(1);
  const int &m = *(apacMemeBloc__m_0);
  const int *const apacMemeBloc__n_0 = new const int(i);
  const int &n = *(apacMemeBloc__n_0);

  int &o = i;
  int &p = x;

  const int *const apacMemeBloc__r_0 = new const int(1);
  const int &r = *(apacMemeBloc__r_0);
  const int &s = i;

  const int *apacMemeBloc__t_0 = new const int[5]{1, 2, 3, 4, 5};
  const int *&t = (apacMemeBloc__t_0);
  const int *apacMemeBloc__u_0 = new const int[2]{1, 2};
  const int *&u = (apacMemeBloc__u_0);
  const int *apacMemeBloc__v_0 = new const int[1]{0};
  const int *&v = (apacMemeBloc__v_0);

  int *apacMemeBloc__w_0 = new int[5]{1, 2, 3, 4, 5};
  int *&w = (apacMemeBloc__w_0);
  int *apacMemeBloc__y_0 = new int[2]{1, 2};
  int *&y = (apacMemeBloc__y_0);
  int *apacMemeBloc__z_0 = new int[1]{0};
  int *&z = (apacMemeBloc__z_0);

  int *aa = &i;
  int *ab = &x;
  int *ac = 0;

  const int *ad = &i;
  const int *ae = &i;
  const int *const af = &x;

  const int *const ag = af;

  const int *const ah = &x;

  int **apacMemeBloc__po_0 = new int *[10];
  int **&po = (apacMemeBloc__po_0);
  int **apacMemeBloc__pl_0 = new int *[2] { aa, ab };
  int **&pl = (apacMemeBloc__pl_0);

  int **apacMemeBloc__pg_0 = new int *[2] { aa, ab };
  int **&pg = (apacMemeBloc__pg_0);

  int *ai = new int(1);

  int(*apacMemeBloc__aj_0)[2][2] =
      new int[2][2][2]{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
  int(*&aj)[2][2] = (apacMemeBloc__aj_0);

  delete apacMemeBloc__a_0;
  delete apacMemeBloc__b_0;
  delete apacMemeBloc__c_0;
  delete apacMemeBloc__d_0;
  delete apacMemeBloc__f_0;
  delete[] apacMemeBloc__h_0;
  delete[] apacMemeBloc__j_0;
  delete apacMemeBloc__k_0;
  delete apacMemeBloc__l_0;
  delete apacMemeBloc__m_0;
  delete apacMemeBloc__n_0;
  delete apacMemeBloc__r_0;
  delete[] apacMemeBloc__t_0;
  delete[] apacMemeBloc__u_0;
  delete[] apacMemeBloc__v_0;
  delete[] apacMemeBloc__w_0;
  delete[] apacMemeBloc__y_0;
  delete[] apacMemeBloc__z_0;
  delete[] apacMemeBloc__po_0;
  delete[] apacMemeBloc__pl_0;
  delete[] apacMemeBloc__pg_0;
  delete[] apacMemeBloc__aj_0;
  return 0;
}