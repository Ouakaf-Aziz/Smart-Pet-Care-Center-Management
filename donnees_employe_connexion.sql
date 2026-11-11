-- A executer une fois dans Oracle (SQL Developer) : rend le compte 1 utilisable avec le module Employe d'origine
-- (statut 'Actif', poste officiel 'Responsable RH' = tous les boutons et droits).
UPDATE EMPLOYE SET STATUT = 'Actif', POSTE = 'Responsable RH' WHERE ID_EMPLOYR = 1;
COMMIT;
